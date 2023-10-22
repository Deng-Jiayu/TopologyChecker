#include "polygonoverlapcheck.h"
#include "checkcontext.h"
#include "qgsgeometryengine.h"
#include "featurepool.h"
#include "qgsvectorlayer.h"
#include "qgsfeedback.h"
#include "qgsapplication.h"

PolygonOverlapCheck::PolygonOverlapCheck(const CheckContext *context, const QVariantMap &configuration)
    : Check(context, configuration), mOverlapThresholdMapUnits(configurationValue<double>(QStringLiteral("areaMax")))
{
    QVariant var = configuration.value("layersA");
    layers = var.value<QVector<QgsVectorLayer *>>();
}

void PolygonOverlapCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
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

        const QgsGeometry geomA = layerFeatureA.geometry();
        QgsRectangle bboxA = geomA.boundingBox();
        std::unique_ptr<QgsGeometryEngine> geomEngineA = CheckerUtils::createGeomEngine(geomA.constGet(), mContext->tolerance);
        geomEngineA->prepareGeometry();
        if (!geomEngineA->isValid())
        {
            messages.append(tr("Overlap check failed for (%1): the geometry is invalid").arg(layerFeatureA.id()));
            continue;
        }

        const CheckerUtils::LayerFeatures layerFeaturesB(featurePools, QList<QString>() << layerFeatureA.layer()->id(), bboxA, compatibleGeometryTypes(), mContext);
        for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
        {
            if (std::find(layers.begin(), layers.end(), layerFeatureB.layer()) == layers.end())
                continue;
            if (feedback && feedback->isCanceled())
                break;

            // > : only report overlaps within same layer once
            if (layerFeatureA.layerId() == layerFeatureB.layerId() && layerFeatureB.feature().id() >= layerFeatureA.feature().id())
            {
                continue;
            }

            QString errMsg;
            const QgsGeometry geometryB = layerFeatureB.geometry();
            const QgsAbstractGeometry *geomB = geometryB.constGet();
            if (geomEngineA->overlaps(geomB, &errMsg))
            {
                std::unique_ptr<QgsAbstractGeometry> interGeom(geomEngineA->intersection(geomB));
                if (interGeom && !interGeom->isEmpty())
                {
                    CheckerUtils::filter1DTypes(interGeom.get());
                    for (int iPart = 0, nParts = interGeom->partCount(); iPart < nParts; ++iPart)
                    {
                        QgsAbstractGeometry *interPart = CheckerUtils::getGeomPart(interGeom.get(), iPart);
                        double area = interPart->area();
                        if (area > mContext->reducedTolerance && (area < mOverlapThresholdMapUnits || mOverlapThresholdMapUnits == 0.0))
                        {
                            errors.append(new PolygonOverlapCheckError(this, layerFeatureA, QgsGeometry(interPart->clone()), interPart->centroid(), area, layerFeatureB));
                        }
                    }
                }
                else if (!errMsg.isEmpty())
                {
                    messages.append(tr("Overlap check between features %1 and %2 %3").arg(layerFeatureA.id(), layerFeatureB.id(), errMsg));
                }
            }
        }
    }
}

void PolygonOverlapCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    QString errMsg;
    PolygonOverlapCheckError *overlapError = static_cast<PolygonOverlapCheckError *>(error);

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

    // Check if error still applies
    CheckerUtils::LayerFeature layerFeatureA(featurePoolA, featureA, mContext, true);
    CheckerUtils::LayerFeature layerFeatureB(featurePoolB, featureB, mContext, true);
    const QgsGeometry geometryA = layerFeatureA.geometry();
    std::unique_ptr<QgsGeometryEngine> geomEngineA = CheckerUtils::createGeomEngine(geometryA.constGet(), mContext->reducedTolerance);
    geomEngineA->prepareGeometry();

    const QgsGeometry geometryB = layerFeatureB.geometry();
    if (!geomEngineA->overlaps(geometryB.constGet()))
    {
        error->setObsolete();
        return;
    }
    std::unique_ptr<QgsAbstractGeometry> interGeom(geomEngineA->intersection(geometryB.constGet(), &errMsg));
    if (!interGeom)
    {
        error->setFixFailed(tr("Failed to compute intersection between overlapping features: %1").arg(errMsg));
        return;
    }

    // Search which overlap part this error parametrizes (using fuzzy-matching of the area and centroid...)
    QgsAbstractGeometry *interPart = nullptr;
    for (int iPart = 0, nParts = interGeom->partCount(); iPart < nParts; ++iPart)
    {
        QgsAbstractGeometry *part = CheckerUtils::getGeomPart(interGeom.get(), iPart);
        if (std::fabs(part->area() - overlapError->value().toDouble()) < mContext->reducedTolerance &&
            CheckerUtils::pointsFuzzyEqual(part->centroid(), overlapError->location(), mContext->reducedTolerance))
        {
            interPart = part;
            break;
        }
    }
    if (!interPart || interPart->isEmpty())
    {
        error->setObsolete();
        return;
    }

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == Subtract)
    {
        std::unique_ptr<QgsAbstractGeometry> diff1(geomEngineA->difference(interPart, &errMsg));
        if (!diff1 || diff1->isEmpty())
        {
            diff1.reset();
        }
        else
        {
            CheckerUtils::filter1DTypes(diff1.get());
        }
        std::unique_ptr<QgsGeometryEngine> geomEngineB = CheckerUtils::createGeomEngine(geometryB.constGet(), mContext->reducedTolerance);
        geomEngineB->prepareGeometry();
        std::unique_ptr<QgsAbstractGeometry> diff2(geomEngineB->difference(interPart, &errMsg));
        if (!diff2 || diff2->isEmpty())
        {
            diff2.reset();
        }
        else
        {
            CheckerUtils::filter1DTypes(diff2.get());
        }
        double shared1 = diff1 ? CheckerUtils::sharedEdgeLength(diff1.get(), interPart, mContext->reducedTolerance) : 0;
        double shared2 = diff2 ? CheckerUtils::sharedEdgeLength(diff2.get(), interPart, mContext->reducedTolerance) : 0;
        if (!diff1 || !diff2 || shared1 == 0. || shared2 == 0.)
        {
            error->setFixFailed(tr("Could not find shared edges between intersection and overlapping features"));
        }
        else
        {
            if (shared1 < shared2)
            {
                QgsCoordinateTransform ct(featurePoolA->crs(), mContext->mapCrs, mContext->transformContext);
                diff1->transform(ct, QgsCoordinateTransform::ReverseTransform);
                featureA.setGeometry(QgsGeometry(std::move(diff1)));

                changes[error->layerId()][featureA.id()].append(Change(ChangeFeature, ChangeChanged));
                featurePoolA->updateFeature(featureA);
            }
            else
            {
                QgsCoordinateTransform ct(featurePoolB->crs(), mContext->mapCrs, mContext->transformContext);
                diff2->transform(ct, QgsCoordinateTransform::ReverseTransform);
                featureB.setGeometry(QgsGeometry(std::move(diff2)));

                changes[overlapError->overlappedFeature().layerId()][featureB.id()].append(Change(ChangeFeature, ChangeChanged));
                featurePoolB->updateFeature(featureB);
            }

            error->setFixed(method);
        }
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList PolygonOverlapCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("移除重复区域")
                                 << QStringLiteral("无");
    return methods;
}

QString PolygonOverlapCheck::description() const
{
    return factoryDescription();
}

QString PolygonOverlapCheck::id() const
{
    return factoryId();
}

Check::Flags PolygonOverlapCheck::flags() const
{
    return factoryFlags();
}

QString PolygonOverlapCheck::factoryDescription()
{
    return QStringLiteral("一个面数据集中存在相互重叠的面对象");
}

Check::CheckType PolygonOverlapCheck::factoryCheckType()
{
    return Check::LayerCheck;
}

QString PolygonOverlapCheck::factoryId()
{
    return QStringLiteral("QgsGeometryPolygonOverlapCheck");
}

Check::Flags PolygonOverlapCheck::factoryFlags()
{
    return Check::AvailableInValidation;
}

QList<QgsWkbTypes::GeometryType> PolygonOverlapCheck::factoryCompatibleGeometryTypes()
{
    return {QgsWkbTypes::PolygonGeometry};
}

bool PolygonOverlapCheck::factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP
{
    return factoryCompatibleGeometryTypes().contains(layer->geometryType());
}

PolygonOverlapCheckError::PolygonOverlapCheckError(const Check *check, const CheckerUtils::LayerFeature &layerFeature, const QgsGeometry &geometry, const QgsPointXY &errorLocation, const QVariant &value, const CheckerUtils::LayerFeature &overlappedFeature)
    : CheckError(check, layerFeature.layer()->id(), layerFeature.feature().id(), geometry, errorLocation, QgsVertexId(), value, ValueArea), mOverlappedFeature(OverlappedFeature(overlappedFeature.layer(), overlappedFeature.feature().id()))
{
}

bool PolygonOverlapCheckError::isEqual(CheckError *other) const
{
    PolygonOverlapCheckError *err = dynamic_cast<PolygonOverlapCheckError *>(other);
    return err &&
           other->layerId() == layerId() &&
           other->featureId() == featureId() &&
           err->overlappedFeature() == overlappedFeature() &&
           CheckerUtils::pointsFuzzyEqual(location(), other->location(), mCheck->context()->reducedTolerance) &&
           std::fabs(value().toDouble() - other->value().toDouble()) < mCheck->context()->reducedTolerance;
}

bool PolygonOverlapCheckError::closeMatch(CheckError *other) const
{
    PolygonOverlapCheckError *err = dynamic_cast<PolygonOverlapCheckError *>(other);
    return err && other->layerId() == layerId() && other->featureId() == featureId() && err->overlappedFeature() == overlappedFeature();
}

bool PolygonOverlapCheckError::handleChanges(const Check::Changes &changes)
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

QString PolygonOverlapCheckError::description() const
{
    return QCoreApplication::translate("QgsGeometryTypeCheckError", "Overlap with %1 at feature %2").arg(mOverlappedFeature.layerName(), QString::number(mOverlappedFeature.featureId()));
}

QMap<QString, QgsFeatureIds> PolygonOverlapCheckError::involvedFeatures() const
{
    QMap<QString, QgsFeatureIds> features;
    features[layerId()].insert(featureId());
    features[mOverlappedFeature.layerId()].insert(mOverlappedFeature.featureId());
    return features;
}

QIcon PolygonOverlapCheckError::icon() const
{

    if (status() == CheckError::StatusFixed)
        return QgsApplication::getThemeIcon(QStringLiteral("/algorithms/mAlgorithmCheckGeometry.svg"));
    else
        return QgsApplication::getThemeIcon(QStringLiteral("/checks/Overlap.svg"));
}
