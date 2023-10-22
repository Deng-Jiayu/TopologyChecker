#include "linecoveredbylinecheck.h"
#include "checkerutils.h"
#include "checkerror.h"
#include "checkerutils.h"

void LineCoveredByLineCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    for (const CheckerUtils::LayerFeature &layerFeatureA : layerFeaturesA)
    {
        if (std::find(layersA.begin(), layersA.end(), layerFeatureA.layer()) == layersA.end())
            continue;

        // TODO, use QgsGeometryEngine
        QVector<QgsGeometry> geomsA = layerFeatureA.geometry().asGeometryCollection();
        for (int iPart = 0; iPart < geomsA.size(); ++iPart)
        {
            QgsGeometry line = geomsA[iPart];

            if (line.isEmpty())
                continue;

            QgsGeometry boundarys;
            featureIds = allLayerFeatureIds(featurePools);
            CheckerUtils::LayerFeatures layerFeaturesB(featurePools, featureIds.keys(), line.boundingBox(), {QgsWkbTypes::LineGeometry}, mContext);
            for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
            {
                if (std::find(layersB.begin(), layersB.end(), layerFeatureB.layer()) == layersB.end())
                    continue;
                QVector<QgsGeometry> geomsB = layerFeatureB.geometry().asGeometryCollection();

                for (int jPart = 0; jPart < geomsB.size(); ++jPart)
                {
                    QgsPolyline tmpLine;
                    for (auto it = geomsB[jPart].vertices_begin(); it != geomsB[jPart].vertices_end(); ++it)
                        tmpLine.push_back(*it);

                    QgsGeometry boundary = QgsGeometry::fromPolyline(tmpLine);
                    boundary = boundary.buffer(mContext->tolerance, 5, QgsGeometry::CapRound, QgsGeometry::JoinStyleRound, 2);
                    if (boundarys.isEmpty())
                    {
                        boundarys = boundary;
                    }
                    else
                    {
                        boundarys = boundarys.combine(boundary);
                    }
                }
            }

            if (!line.within(boundarys))
            {
                errors.append(new CheckError(this, layerFeatureA, layerFeatureA.geometry().centroid().asPoint(), QgsVertexId(iPart)));
            }
        }
    }
}

void LineCoveredByLineCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const
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
    QgsGeometry line = feature.geometry().asGeometryCollection()[vidx.part];
    if (line.isEmpty())
    {
        error->setObsolete();
        return;
    }
    QgsGeometry boundarys;
    QMap<QString, QgsFeatureIds> featureIds = allLayerFeatureIds(featurePools);
    CheckerUtils::LayerFeatures layerFeaturesB(featurePools, featureIds.keys(), line.boundingBox(), {QgsWkbTypes::PolygonGeometry}, mContext);
    for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
    {
        if (std::find(layersB.begin(), layersB.end(), layerFeatureB.layer()) == layersB.end())
            continue;
        QVector<QgsGeometry> geomsB = layerFeatureB.geometry().asGeometryCollection();

        for (int jPart = 0; jPart < geomsB.size(); ++jPart)
        {
            QgsPolyline tmpLine;
            for (auto it = geomsB[jPart].vertices_begin(); it != geomsB[jPart].vertices_end(); ++it)
                tmpLine.push_back(*it);

            QgsGeometry boundary = QgsGeometry::fromPolyline(tmpLine);
            boundary = boundary.buffer(mContext->tolerance, 5, QgsGeometry::CapRound, QgsGeometry::JoinStyleRound, 2);
            if (boundarys.isEmpty())
                boundarys = boundary;
            else
                boundarys = boundarys.combine(boundary);
        }
    }
    if (line.within(boundarys))
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
        deleteFeatureGeometryPart(featurePools, error->layerId(), feature, vidx.part, changes);
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}
