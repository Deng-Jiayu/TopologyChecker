#include "checkcontext.h"
#include "lineoverlapcheck.h"
#include "featurepool.h"
#include "qgsvectorlayer.h"
#include "qgsfeedback.h"
#include "qgsapplication.h"

LineOverlapCheck::LineOverlapCheck(const CheckContext *context, const QVariantMap &configuration)
    : Check(context, configuration)
{
    QVariant var = configuration.value("layersA");
    layers = var.value<QVector<QgsVectorLayer *>>();
}

void LineOverlapCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    const CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    QList<QString> layerIds = featureIds.keys();
    for (const CheckerUtils::LayerFeature &layerFeatureA : layerFeaturesA)
    {
        if (std::find(layers.begin(), layers.end(), layerFeatureA.layer()) == layers.end())
            continue;
        if (feedback && feedback->isCanceled())
            break;

        // Ensure each pair of layers only gets compared once: remove the current layer from the layerIds, but add it to the layerList for layerFeaturesB
        layerIds.removeOne(layerFeatureA.layer()->id());
        const QgsGeometry geometryA = layerFeatureA.geometry();
        const QgsAbstractGeometry *geomA = geometryA.constGet();
        for (int iPart = 0, iParts = geomA->partCount(); iPart < iParts; ++iPart)
        {
            const QgsAbstractGeometry *lineA = CheckerUtils::getGeomPart(geomA, iPart);
            if (lineA->vertexCount() <= 1)
                continue;

            QVector<LineSegment> linesA;
            auto i = lineA->vertices_begin();
            QgsPoint first = *i;
            for (++i; i != lineA->vertices_end(); ++i)
            {
                LineSegment line(first, *i);
                linesA.push_back(line);
                first = *i;
            }
            std::sort(linesA.begin(), linesA.end(), CheckerUtils::cmp);

            QgsRectangle bboxA = lineA->boundingBox();
            const CheckerUtils::LayerFeatures layerFeaturesB(featurePools, QList<QString>() << layerFeatureA.layer()->id(), bboxA, compatibleGeometryTypes(), mContext);
            for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
            {
                if (feedback && feedback->isCanceled())
                    break;
                // > : only report overlaps within same layer once
                if (layerFeatureB.feature().id() >= layerFeatureA.feature().id())
                    continue;

                const QgsGeometry geometryB = layerFeatureB.geometry();
                const QgsAbstractGeometry *geomB = geometryB.constGet();
                for (int jPart = 0, jParts = geomB->partCount(); jPart < jParts; ++jPart)
                {
                    const QgsAbstractGeometry *lineB = CheckerUtils::getGeomPart(geomB, jPart);
                    if (lineB->vertexCount() <= 1)
                        continue;

                    QVector<LineSegment> linesB;
                    auto j = lineB->vertices_begin();
                    QgsPoint first = *j;
                    for (++j; j != lineB->vertices_end(); ++j)
                    {
                        LineSegment line(first, *j);
                        linesB.push_back(line);
                        first = *j;
                    }
                    std::sort(linesB.begin(), linesB.end(), CheckerUtils::cmp);

                    QVector<QgsGeometry> ans = CheckerUtils::lineOverlay(linesA, linesB, mContext->tolerance);
                    if (ans.empty())
                        continue;
                    QgsGeometry conflict;
                    for (auto k = ans.begin(); k != ans.end(); ++k)
                    {
                        if (conflict.isEmpty())
                            conflict = *k;
                        else
                            conflict = conflict.combine(*k);
                    }
                    errors.append(new LineOverlapCheckError(this, layerFeatureA, conflict, conflict.centroid().asPoint(),
                                                            layerFeatureB, QgsVertexId(iPart), QgsVertexId(jPart)));
                }
            }
        }
    }
}

void LineOverlapCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    LineOverlapCheckError *overlapError = static_cast<LineOverlapCheckError *>(error);

    FeaturePool *featurePoolA = featurePools[overlapError->layerId()];
    FeaturePool *featurePoolB = featurePools[overlapError->overlappedFeature().layerId()];
    QgsFeature featureA;
    QgsFeature featureB;
    if (!featurePoolA->getFeature(overlapError->featureId(), featureA) ||
        !featurePoolB->getFeature(overlapError->overlappedFeature().featureId(), featureB))
    {
        error->setObsolete();
        return;
    }

    // Check if line still exists
    QgsVertexId vidx = overlapError->vidx();
    QgsVertexId vidy = overlapError->overlappedFeature().vidx();
    const QgsAbstractGeometry *geomA = featureA.geometry().constGet();
    const QgsAbstractGeometry *geomB = featureB.geometry().constGet();
    if (!vidx.isValid(geomA) || !vidy.isValid(geomB))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsAbstractGeometry *lineA = CheckerUtils::getGeomPart(geomA, vidx.part);
    if (lineA->vertexCount() <= 1)
    {
        error->setObsolete();
        return;
    }
    QVector<LineSegment> linesA;
    auto i = lineA->vertices_begin();
    QgsPoint first = *i;
    for (++i; i != lineA->vertices_end(); ++i)
    {
        LineSegment line(first, *i);
        linesA.push_back(line);
        first = *i;
    }
    std::sort(linesA.begin(), linesA.end(), CheckerUtils::cmp);
    const QgsAbstractGeometry *lineB = CheckerUtils::getGeomPart(geomB, vidy.part);
    if (lineB->vertexCount() <= 1)
    {
        error->setObsolete();
        return;
    }
    QVector<LineSegment> linesB;
    auto j = lineB->vertices_begin();
    first = *j;
    for (++j; j != lineB->vertices_end(); ++j)
    {
        LineSegment line(first, *j);
        linesB.push_back(line);
        first = *j;
    }
    std::sort(linesB.begin(), linesB.end(), CheckerUtils::cmp);
    QVector<QgsGeometry> ans = CheckerUtils::lineOverlay(linesA, linesB, mContext->tolerance);
    if (ans.empty())
    {
        error->setObsolete();
        return;
    }

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == Delete)
    {
        deleteFeatureGeometryPart(featurePools, error->layerId(), featureA, error->vidx().part, changes);
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList LineOverlapCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("删除要素")
                                 << QStringLiteral("无");
    return methods;
}

QString LineOverlapCheck::description() const
{
    return factoryDescription();
}

QString LineOverlapCheck::id() const
{
    return factoryId();
}

Check::Flags LineOverlapCheck::flags() const
{
    return factoryFlags();
}

QString LineOverlapCheck::factoryDescription()
{
    return QStringLiteral("一个线数据集中线对象之间有重叠");
}

Check::CheckType LineOverlapCheck::factoryCheckType()
{
    return Check::LayerCheck;
}

QString LineOverlapCheck::factoryId()
{
    return QStringLiteral("LineOverlapCheck");
}

Check::Flags LineOverlapCheck::factoryFlags()
{
    return Check::AvailableInValidation;
}

QList<QgsWkbTypes::GeometryType> LineOverlapCheck::factoryCompatibleGeometryTypes()
{
    return {QgsWkbTypes::LineGeometry};
}

bool LineOverlapCheck::factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP
{
    return factoryCompatibleGeometryTypes().contains(layer->geometryType());
}

LineOverlapCheckError::LineOverlapCheckError(const Check *check, const CheckerUtils::LayerFeature &layerFeature, const QgsGeometry &geometry, const QgsPointXY &errorLocation, const CheckerUtils::LayerFeature &overlappedFeature, const QgsVertexId &vida, const QgsVertexId &vidb)
    : CheckError(check, layerFeature.layer()->id(), layerFeature.feature().id(), geometry, errorLocation, vida),
      mOverlappedFeature(OverlappedFeature(overlappedFeature.layer(), overlappedFeature.feature().id(), vidb))
{
}

bool LineOverlapCheckError::isEqual(CheckError *other) const
{
    LineOverlapCheckError *err = dynamic_cast<LineOverlapCheckError *>(other);
    return err &&
           other->layerId() == layerId() &&
           other->featureId() == featureId() &&
           err->overlappedFeature() == overlappedFeature() &&
           CheckerUtils::pointsFuzzyEqual(location(), other->location(), mCheck->context()->reducedTolerance) &&
           std::fabs(value().toDouble() - other->value().toDouble()) < mCheck->context()->reducedTolerance;
}

bool LineOverlapCheckError::closeMatch(CheckError *other) const
{
    LineOverlapCheckError *err = dynamic_cast<LineOverlapCheckError *>(other);
    return err && other->layerId() == layerId() && other->featureId() == featureId() && err->overlappedFeature() == overlappedFeature();
}

bool LineOverlapCheckError::handleChanges(const Check::Changes &changes)
{
    if (!CheckError::handleChanges(changes))
    {
        return false;
    }
    if (changes.value(mOverlappedFeature.layerId()).keys().contains(mOverlappedFeature.featureId()))
    {
        return false;
    }
    return true;
}

QString LineOverlapCheckError::description() const
{
    return QCoreApplication::translate("QgsGeometryTypeCheckError", "Overlap with feature %1").arg(QString::number(mOverlappedFeature.featureId()));
}

QMap<QString, QgsFeatureIds> LineOverlapCheckError::involvedFeatures() const
{
    QMap<QString, QgsFeatureIds> features;
    features[layerId()].insert(featureId());
    features[mOverlappedFeature.layerId()].insert(mOverlappedFeature.featureId());
    return features;
}

QIcon LineOverlapCheckError::icon() const
{

    if (status() == CheckError::StatusFixed)
        return QgsApplication::getThemeIcon(QStringLiteral("/algorithms/mAlgorithmCheckGeometry.svg"));
    else
        return QgsApplication::getThemeIcon(QStringLiteral("/checks/Overlap.svg"));
}
