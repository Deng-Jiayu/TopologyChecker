#include "polygoninpolygoncheck.h"
#include "checkcontext.h"
#include "qgsgeometryengine.h"
#include "featurepool.h"
#include "qgsvectorlayer.h"

void PolygonInPolygonCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);
    for (const CheckerUtils::LayerFeature &layerFeatureA : layerFeaturesA)
    {
        if (std::find(layersA.begin(), layersA.end(), layerFeatureA.layer()) == layersA.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeatureA.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            const QgsAbstractGeometry *geomA = CheckerUtils::getGeomPart(geom, iPart);
            QgsRectangle bboxA = geomA->boundingBox();
            bool contains = false;
            CheckerUtils::LayerFeatures layerFeaturesB(featurePools, featureIds.keys(), bboxA, compatibleGeometryTypes(), mContext);
            for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
            {
                if (std::find(layersB.begin(), layersB.end(), layerFeatureB.layer()) == layersB.end())
                    continue;
                const QgsAbstractGeometry *testGeom = layerFeatureB.geometry().constGet();
                for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
                {
                    const QgsAbstractGeometry *geomB = CheckerUtils::getGeomPart(testGeom, jPart);
                    std::unique_ptr<QgsGeometryEngine> geomEngineB = CheckerUtils::createGeomEngine(geomB, mContext->tolerance);
                    if (!geomEngineB->isValid())
                    {
                        messages.append(tr("Polygon in polygon check failed for (%1): the geometry is invalid").arg(layerFeatureB.id()));
                        continue;
                    }

                    QString errMsg;
                    if (geomEngineB->contains(geomA, &errMsg) && errMsg.isEmpty())
                    {
                        contains = true;
                        break;
                    }
                }
                if(contains) break;
            }
            if(!contains) {
                errors.append(new CheckError(this, layerFeatureA, geomA->centroid(), QgsVertexId(iPart)));
            }
        }
    }
}

void PolygonInPolygonCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }

    const QgsGeometry g = feature.geometry();
    const QgsAbstractGeometry *geom = g.constGet();
    QgsVertexId vidx = error->vidx();
    // Check if polygon still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsAbstractGeometry *geomA = CheckerUtils::getGeomPart(geom, vidx.part);
    QgsRectangle bboxA = geomA->boundingBox();
    QMap<QString, QgsFeatureIds> featureIds = allLayerFeatureIds(featurePools);
    CheckerUtils::LayerFeatures layerFeaturesB(featurePools, featureIds.keys(), bboxA, compatibleGeometryTypes(), mContext);
    for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
    {
        if (std::find(layersB.begin(), layersB.end(), layerFeatureB.layer()) == layersB.end())
            continue;
        const QgsAbstractGeometry *testGeom = layerFeatureB.geometry().constGet();
        for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
        {
            const QgsAbstractGeometry *geomB = CheckerUtils::getGeomPart(testGeom, jPart);
            std::unique_ptr<QgsGeometryEngine> geomEngineB = CheckerUtils::createGeomEngine(geomB, mContext->tolerance);
            if (!geomEngineB->isValid())
            {
                continue;
            }

            QString errMsg;
            if (geomEngineB->contains(geomA, &errMsg) && errMsg.isEmpty())
            {
                error->setObsolete();
                return;
            }
        }
    }

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == Delete)
    {
        deleteFeatureGeometryPart(featurePools, error->layerId(), feature, vidx.part, changes);
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList PolygonInPolygonCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("删除要素")
                                 << QStringLiteral("无");
    return methods;
}
