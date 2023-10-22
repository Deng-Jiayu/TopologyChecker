#include "checkcontext.h"
#include "qgsgeometryengine.h"
#include "qgsgeometrycollection.h"
#include "areacheck.h"
#include "featurepool.h"
#include "checkerror.h"

void AreaCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        double layerToMapUnits = scaleFactor(layerFeature.layer());
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            double value;
            const QgsAbstractGeometry *part = CheckerUtils::getGeomPart(geom, iPart);
            if (checkThreshold(layerToMapUnits, part, value))
            {
                errors.append(new CheckError(this, layerFeature, part->centroid(), QgsVertexId(iPart), value * layerToMapUnits * layerToMapUnits, CheckError::ValueArea));
            }
        }
    }
}

void AreaCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const
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

    double layerToMapUnits = scaleFactor(featurePool->layer());

    // Check if polygon still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    double value;
    if (!checkThreshold(layerToMapUnits, CheckerUtils::getGeomPart(geom, vidx.part), value))
    {
        error->setObsolete();
        return;
    }

    // Fix with selected method
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == Delete)
    {
        deleteFeatureGeometryPart(featurePools, error->layerId(), feature, vidx.part, changes);
        error->setFixed(method);
    }
    else if (method == MergeLongestEdge || method == MergeLargestArea)
    {
        QString errMsg;
        if (mergeWithNeighbor(featurePools, error->layerId(), feature, vidx.part, method, mergeAttributeIndices[error->layerId()], changes, errMsg))
        {
            error->setFixed(method);
        }
        else
        {
            error->setFixFailed(tr("Failed to merge with neighbor: %1").arg(errMsg));
        }
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

bool AreaCheck::checkThreshold(double layerToMapUnits, const QgsAbstractGeometry *geom, double &value) const
{
    value = geom->area();
    double low = mAreaMin / (layerToMapUnits * layerToMapUnits);
    double high = mAreaMax / (layerToMapUnits * layerToMapUnits);
    if (value >= low && value <= high)
        return true;
    else
        return false;
}

bool AreaCheck::mergeWithNeighbor(const QMap<QString, FeaturePool *> &featurePools,
                                  const QString &layerId, QgsFeature &feature,
                                  int partIdx, int method, int mergeAttributeIndex, Changes &changes, QString &errMsg) const
{
    FeaturePool *featurePool = featurePools[layerId];

    double maxVal = 0.;
    QgsFeature mergeFeature;
    int mergePartIdx = -1;
    bool matchFound = false;
    QgsGeometry featureGeometry = feature.geometry();
    const QgsAbstractGeometry *geom = featureGeometry.constGet();

    // Search for touching neighboring geometries
    const QgsFeatureIds intersects = featurePool->getIntersects(featureGeometry.boundingBox());
    for (QgsFeatureId testId : intersects)
    {
        QgsFeature testFeature;
        if (!featurePool->getFeature(testId, testFeature))
        {
            continue;
        }
        QgsGeometry testFeatureGeom = testFeature.geometry();
        const QgsAbstractGeometry *testGeom = testFeatureGeom.constGet();
        for (int testPartIdx = 0, nTestParts = testGeom->partCount(); testPartIdx < nTestParts; ++testPartIdx)
        {
            if (testId == feature.id() && testPartIdx == partIdx)
            {
                continue;
            }
            double len = CheckerUtils::sharedEdgeLength(CheckerUtils::getGeomPart(geom, partIdx), CheckerUtils::getGeomPart(testGeom, testPartIdx), mContext->reducedTolerance);
            if (len > 0.)
            {
                if (method == MergeLongestEdge || method == MergeLargestArea)
                {
                    double val;
                    if (method == MergeLongestEdge)
                    {
                        val = len;
                    }
                    else
                    {
                        if (dynamic_cast<const QgsGeometryCollection *>(testGeom))
                            val = static_cast<const QgsGeometryCollection *>(testGeom)->geometryN(testPartIdx)->area();
                        else
                            val = testGeom->area();
                    }
                    if (val > maxVal)
                    {
                        maxVal = val;
                        mergeFeature = testFeature;
                        mergePartIdx = testPartIdx;
                    }
                }
            }
        }
        if (matchFound)
        {
            break;
        }
    }

    // Merge geometries
    QgsGeometry mergeFeatureGeom = mergeFeature.geometry();
    const QgsAbstractGeometry *mergeGeom = mergeFeatureGeom.constGet();
    std::unique_ptr<QgsGeometryEngine> geomEngine(CheckerUtils::createGeomEngine(CheckerUtils::getGeomPart(mergeGeom, mergePartIdx), mContext->reducedTolerance));
    QgsAbstractGeometry *combinedGeom = geomEngine->combine(CheckerUtils::getGeomPart(geom, partIdx), &errMsg);
    if (!combinedGeom || combinedGeom->isEmpty() || !QgsWkbTypes::isSingleType(combinedGeom->wkbType()))
    {
        return false;
    }

    // Replace polygon in merge geometry
    if (mergeFeature.id() == feature.id() && mergePartIdx > partIdx)
    {
        --mergePartIdx;
    }
    replaceFeatureGeometryPart(featurePools, layerId, mergeFeature, mergePartIdx, combinedGeom, changes);
    // Remove polygon from source geometry
    deleteFeatureGeometryPart(featurePools, layerId, feature, partIdx, changes);

    return true;
}

QStringList AreaCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("与共享边最长的相邻多边形合并")
                                 << QStringLiteral("与面积最大的相邻多边形合并")
                                 << QStringLiteral("删除要素")
                                 << QStringLiteral("无");
    return methods;
}
