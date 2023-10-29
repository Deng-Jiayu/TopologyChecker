#include "checkcontext.h"
#include "linelayeroverlapcheck.h"
#include "featurepool.h"
#include "qgsvectorlayer.h"
#include "qgsfeedback.h"
#include "qgsapplication.h"
#include "linesegment.h"
#include "checkerutils.h"

LineLayerOverlapCheck::LineLayerOverlapCheck(const CheckContext *context, const QVariantMap &configuration)
    : Check(context, configuration)
{
    QVariant var = configuration.value("layersA");
    layersA = var.value<QSet<QgsVectorLayer *>>();
    var = configuration.value("layersB");
    layersB = var.value<QSet<QgsVectorLayer *>>();
}

void LineLayerOverlapCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    const CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    QList<QString> layerIds = featureIds.keys();
    for (const CheckerUtils::LayerFeature &layerFeatureA : layerFeaturesA)
    {
        if (std::find(layersA.begin(), layersA.end(), layerFeatureA.layer()) == layersA.end())
            continue;
        if (feedback && feedback->isCanceled())
            break;

        layerIds.removeOne(layerFeatureA.layer()->id());

        const QgsAbstractGeometry *geomA = layerFeatureA.geometry().constGet();
        for (int iPart = 0, iParts = geomA->partCount(); iPart < iParts; ++iPart)
        {
            const QgsAbstractGeometry *lineA = CheckerUtils::getGeomPart(geomA, iPart);
            if (lineA->vertexCount() <= 1)
                continue;

            QVector<LineSegment> linesA;
            auto itA = lineA->vertices_begin();
            QgsPoint first = *itA;
            for (++itA; itA != lineA->vertices_end(); ++itA)
            {
                LineSegment line(first, *itA);
                linesA.push_back(line);
                first = *itA;
            }
            std::sort(linesA.begin(), linesA.end(), CheckerUtils::cmp);

            const CheckerUtils::LayerFeatures layerFeaturesB(featurePools, layerIds, lineA->boundingBox(), compatibleGeometryTypes(), mContext);
            for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
            {
                if (std::find(layersB.begin(), layersB.end(), layerFeatureB.layer()) == layersB.end())
                    continue;
                if (feedback && feedback->isCanceled())
                    break;

                // > : only report overlaps within different layer
                if (layerFeatureA.layerId() == layerFeatureB.layerId())
                    continue;

                const QgsAbstractGeometry *geomB = layerFeatureB.geometry().constGet();
                for (int jPart = 0, jParts = geomB->partCount(); jPart < jParts; ++jPart)
                {
                    const QgsAbstractGeometry *lineB = CheckerUtils::getGeomPart(geomB, jPart);
                    if (lineB->vertexCount() <= 1)
                        continue;

                    QVector<LineSegment> linesB;
                    auto itB = lineB->vertices_begin();
                    QgsPoint first = *itB;
                    for (++itB; itB != lineB->vertices_end(); ++itB)
                    {
                        LineSegment line(first, *itB);
                        linesB.push_back(line);
                        first = *itB;
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
                    errors.append(new LineLayerOverlapCheckError(this, layerFeatureA, conflict, conflict.centroid().asPoint(),
                                                                 layerFeatureB, QgsVertexId(iPart), QgsVertexId(jPart)));
                }
            }
        }
    }
}

void LineLayerOverlapCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    QString errMsg;
    LineLayerOverlapCheckError *overlapError = static_cast<LineLayerOverlapCheckError *>(error);

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

QStringList LineLayerOverlapCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("删除要素")
                                 << QStringLiteral("无");
    return methods;
}

QString LineLayerOverlapCheck::description() const
{
    return factoryDescription();
}

QString LineLayerOverlapCheck::id() const
{
    return factoryId();
}

Check::Flags LineLayerOverlapCheck::flags() const
{
    return factoryFlags();
}

QString LineLayerOverlapCheck::factoryDescription()
{
    return QStringLiteral("两个线数据集之间的线对象有重合");
}

Check::CheckType LineLayerOverlapCheck::factoryCheckType()
{
    return Check::LayerCheck;
}

QString LineLayerOverlapCheck::factoryId()
{
    return QStringLiteral("LineLayerOverlapCheck");
}

Check::Flags LineLayerOverlapCheck::factoryFlags()
{
    return Check::AvailableInValidation;
}

QList<QgsWkbTypes::GeometryType> LineLayerOverlapCheck::factoryCompatibleGeometryTypes()
{
    return {QgsWkbTypes::LineGeometry};
}

bool LineLayerOverlapCheck::factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP
{
    return factoryCompatibleGeometryTypes().contains(layer->geometryType());
}

LineLayerOverlapCheckError::LineLayerOverlapCheckError(const Check *check, const CheckerUtils::LayerFeature &layerFeature, const QgsGeometry &geometry, const QgsPointXY &errorLocation, const CheckerUtils::LayerFeature &overlappedFeature, const QgsVertexId &vida, const QgsVertexId &vidb)
    : CheckError(check, layerFeature.layer()->id(), layerFeature.feature().id(), geometry, errorLocation, vida),
      mOverlappedFeature(OverlappedFeature(overlappedFeature.layer(), overlappedFeature.feature().id(), vidb))
{
}

bool LineLayerOverlapCheckError::isEqual(CheckError *other) const
{
    LineLayerOverlapCheckError *err = dynamic_cast<LineLayerOverlapCheckError *>(other);
    return err &&
           other->layerId() == layerId() &&
           other->featureId() == featureId() &&
           err->overlappedFeature() == overlappedFeature() &&
           CheckerUtils::pointsFuzzyEqual(location(), other->location(), mCheck->context()->reducedTolerance) &&
           std::fabs(value().toDouble() - other->value().toDouble()) < mCheck->context()->reducedTolerance;
}

bool LineLayerOverlapCheckError::closeMatch(CheckError *other) const
{
    LineLayerOverlapCheckError *err = dynamic_cast<LineLayerOverlapCheckError *>(other);
    return err && other->layerId() == layerId() && other->featureId() == featureId() && err->overlappedFeature() == overlappedFeature();
}

bool LineLayerOverlapCheckError::handleChanges(const Check::Changes &changes)
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

QString LineLayerOverlapCheckError::description() const
{
    return QCoreApplication::translate("QgsGeometryTypeCheckError", "Overlap with %1 at feature %2").arg(mOverlappedFeature.layerName(), QString::number(mOverlappedFeature.featureId()));
}

QMap<QString, QgsFeatureIds> LineLayerOverlapCheckError::involvedFeatures() const
{
    QMap<QString, QgsFeatureIds> features;
    features[layerId()].insert(featureId());
    features[mOverlappedFeature.layerId()].insert(mOverlappedFeature.featureId());
    return features;
}

QIcon LineLayerOverlapCheckError::icon() const
{
    if (status() == CheckError::StatusFixed)
        return QgsApplication::getThemeIcon(QStringLiteral("/algorithms/mAlgorithmCheckGeometry.svg"));
    else
        return QgsApplication::getThemeIcon(QStringLiteral("/checks/Overlap.svg"));
}
