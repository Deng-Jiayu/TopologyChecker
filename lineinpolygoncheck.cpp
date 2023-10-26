#include "lineinpolygoncheck.h"
#include "checkerutils.h"
#include "checkerror.h"
#include "checkerutils.h"

void LineInPolygonCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    for (const CheckerUtils::LayerFeature &layerFeatureA : layerFeaturesA)
    {
        if (std::find(lineLayers.begin(), lineLayers.end(), layerFeatureA.layer()) == lineLayers.end())
            continue;

        // TODO, use  QgsGeometryEngine
        QVector<QgsGeometry> geomsA = layerFeatureA.geometry().asGeometryCollection();
        for (int iPart = 0; iPart < geomsA.size(); ++iPart)
        {
            QgsGeometry &line = geomsA[iPart];

            if (line.isEmpty())
                continue;

            featureIds = allLayerFeatureIds(featurePools);
            CheckerUtils::LayerFeatures layerFeaturesB(featurePools, featureIds.keys(), line.boundingBox(), {QgsWkbTypes::PolygonGeometry}, mContext);
            for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
            {
                if (std::find(polygonLayers.begin(), polygonLayers.end(), layerFeatureB.layer()) == polygonLayers.end())
                    continue;
                QVector<QgsGeometry> geomsB = layerFeatureB.geometry().asGeometryCollection();

                for (int jPart = 0; jPart < geomsB.size(); ++jPart)
                {
                    QgsGeometry g = line.intersection(geomsB[jPart]);
                    if(g.isEmpty()) continue;
                    errors.append(new CheckError(this, layerFeatureA.layerId(), layerFeatureA.feature().id(), g, g.centroid().asPoint(), QgsVertexId(iPart)));
                }
            }
        }
    }
}

void LineInPolygonCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }
    QgsGeometry featureGeometry = feature.geometry();
    QgsAbstractGeometry *geometry = featureGeometry.get();
    QgsVertexId vidx = error->vidx();

    // Check if geometry still exists
    if (!vidx.isValid(geometry))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    QVector<QgsGeometry> geomsA = feature.geometry().asGeometryCollection();
    QgsGeometry &line = geomsA[vidx.part];
    if (line.isEmpty()) {
        error->setObsolete();
        return;
    }
    bool touches = false;
    QMap<QString, QgsFeatureIds> featureIds = allLayerFeatureIds(featurePools);
    CheckerUtils::LayerFeatures layerFeaturesB(featurePools, featureIds.keys(), line.boundingBox(), {QgsWkbTypes::PolygonGeometry}, mContext);
    for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
    {
        if (std::find(polygonLayers.begin(), polygonLayers.end(), layerFeatureB.layer()) == polygonLayers.end())
            continue;
        QVector<QgsGeometry> geomsB = layerFeatureB.geometry().asGeometryCollection();
        for (int jPart = 0; jPart < geomsB.size(); ++jPart)
        {
            QgsGeometry g = line.intersection(geomsB[jPart]);
            if (!g.isEmpty()) {
                touches = true;
                break;
            }
        }
        if(touches) break;
    }
    if(!touches) {
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
        deleteFeatureGeometryPart(featurePools, error->layerId(), feature, vidx.part, changes);
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}
