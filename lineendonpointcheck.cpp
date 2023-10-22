#include "lineendonpointcheck.h"
#include "checkerutils.h"
#include "checkerror.h"
#include "qgsgeometryutils.h"

void LineEndOnPointCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, context());
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(lineLayers.begin(), lineLayers.end(), layerFeature.layer()) == lineLayers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            for (int iRing = 0, nRings = geom->ringCount(iPart); iRing < nRings; ++iRing)
            {
                int nVerts = geom->vertexCount(iPart, iRing);
                for (int iVert = 0; iVert < nVerts; ++iVert)
                {
                    if (iVert == 1)
                        iVert = nVerts - 1;
                    const QgsPoint &point = geom->vertexAt(QgsVertexId(iPart, iRing, iVert));
                    // Check that line end on a point
                    bool touches = false;
                    QgsRectangle rect(point.x() - mContext->tolerance, point.y() - mContext->tolerance,
                                      point.x() + mContext->tolerance, point.y() + mContext->tolerance);
                    featureIds = allLayerFeatureIds(featurePools);
                    CheckerUtils::LayerFeatures checkFeatures(featurePools, featureIds.keys(), rect, {QgsWkbTypes::PointGeometry}, mContext);
                    for (const CheckerUtils::LayerFeature &checkFeature : checkFeatures)
                    {
                        if (std::find(pointLayers.begin(), pointLayers.end(), checkFeature.layer()) == pointLayers.end())
                            continue;
                        const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
                        for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
                        {
                            const QgsPoint *p = dynamic_cast<const QgsPoint *>(CheckerUtils::getGeomPart(testGeom, jPart));

                            if (p->distance(point) < mContext->tolerance)
                            {
                                touches = true;
                                break;
                            }
                        }
                        if (touches)
                            break;
                    }
                    if (!touches)
                    {
                        errors.append(new CheckError(this, layerFeature, point, QgsVertexId(iPart, iRing, iVert)));
                    }
                }
            }
        }
    }
}

void LineEndOnPointCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const
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

    // Check if point still exists
    if (!vidx.isValid(geometry))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsPoint &point = geometry->vertexAt(vidx);
    bool touches = false;
    QgsRectangle rect(point.x() - mContext->tolerance, point.y() - mContext->tolerance,
                      point.x() + mContext->tolerance, point.y() + mContext->tolerance);
    QMap<QString, QgsFeatureIds> featureIds = allLayerFeatureIds(featurePools);
    CheckerUtils::LayerFeatures checkFeatures(featurePools, featureIds.keys(), rect, {QgsWkbTypes::PointGeometry}, mContext);
    for (const CheckerUtils::LayerFeature &checkFeature : checkFeatures)
    {
        if (std::find(pointLayers.begin(), pointLayers.end(), checkFeature.layer()) == pointLayers.end())
            continue;
        const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
        for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
        {
            const QgsPoint *p = dynamic_cast<const QgsPoint *>(CheckerUtils::getGeomPart(testGeom, jPart));
            if (p->distance(point) < mContext->tolerance)
            {
                touches = true;
                break;
            }
        }
        if (touches)
            break;
    }
    if(touches) {
        error->setObsolete();
        return;
    }


    // Fix error
    int n = CheckerUtils::polyLineSize(geometry, vidx.part, vidx.ring);
    if (n == 0)
    {
        error->setObsolete();
        return;
    }
    const QgsPoint &p1 = geometry->vertexAt(QgsVertexId(vidx.part, vidx.ring, (vidx.vertex - 1 + n) % n));
    const QgsPoint &p2 = geometry->vertexAt(vidx);
    const QgsPoint &p3 = geometry->vertexAt(QgsVertexId(vidx.part, vidx.ring, (vidx.vertex + 1) % n));

    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == DeleteNode)
    {
        if (!CheckerUtils::canDeleteVertex(geometry, vidx.part, vidx.ring))
        {
            error->setFixFailed(QStringLiteral("操作生成的几何体退化"));
        }
        else if (!geometry->deleteVertex(error->vidx()))
        {
            error->setFixFailed(QStringLiteral("无法删除线端点"));
        }
        else
        {
            changes[error->layerId()][error->featureId()].append(Change(ChangeNode, ChangeRemoved, vidx));
            // Avoid duplicate nodes as result of deleting spike vertex
            if (QgsGeometryUtils::sqrDistance2D(p1, p3) < mContext->tolerance &&
                CheckerUtils::canDeleteVertex(geometry, vidx.part, vidx.ring) &&
                geometry->deleteVertex(error->vidx())) // error->vidx points to p3 after removing p2
            {
                changes[error->layerId()][error->featureId()].append(Change(ChangeNode, ChangeRemoved, QgsVertexId(vidx.part, vidx.ring, (vidx.vertex + 1) % n)));
            }
            feature.setGeometry(featureGeometry);
            featurePool->updateFeature(feature);
            error->setFixed(method);
        }
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
