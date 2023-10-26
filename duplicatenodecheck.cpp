#include "duplicatenodecheck.h"
#include "checkcontext.h"
#include "qgsgeometryutils.h"
#include "featurepool.h"
#include "checkerror.h"

void DuplicateNodeCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            for (int iRing = 0, nRings = geom->ringCount(iPart); iRing < nRings; ++iRing)
            {
                int nVerts = CheckerUtils::polyLineSize(geom, iPart, iRing);
                if (nVerts < 2)
                    continue;
                for (int iVert = nVerts - 1, jVert = 0; jVert < nVerts; iVert = jVert++)
                {
                    QgsPoint pi = geom->vertexAt(QgsVertexId(iPart, iRing, iVert));
                    QgsPoint pj = geom->vertexAt(QgsVertexId(iPart, iRing, jVert));
                    if (QgsGeometryUtils::sqrDistance2D(pi, pj) < mContext->tolerance)
                    {
                        errors.append(new CheckError(this, layerFeature, pj, QgsVertexId(iPart, iRing, jVert)));
                    }
                }
            }
        }
    }
}

void DuplicateNodeCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }
    QgsGeometry featureGeom = feature.geometry();
    QgsAbstractGeometry *geom = featureGeom.get();
    QgsVertexId vidx = error->vidx();

    // Check if point still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    int nVerts = CheckerUtils::polyLineSize(geom, vidx.part, vidx.ring);
    QgsPoint pi = geom->vertexAt(QgsVertexId(vidx.part, vidx.ring, (vidx.vertex + nVerts - 1) % nVerts));
    QgsPoint pj = geom->vertexAt(error->vidx());
    if (QgsGeometryUtils::sqrDistance2D(pi, pj) >= mContext->tolerance)
    {
        error->setObsolete();
        return;
    }

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == RemoveDuplicates)
    {
        if (!CheckerUtils::canDeleteVertex(geom, vidx.part, vidx.ring))
        {
            error->setFixFailed(QStringLiteral("生成的几何无效"));
        }
        else if (!geom->deleteVertex(error->vidx()))
        {
            error->setFixFailed(tr("Failed to delete vertex"));
        }
        else
        {
            feature.setGeometry(featureGeom);
            featurePool->updateFeature(feature);
            error->setFixed(method);
            changes[error->layerId()][error->featureId()].append(Change(ChangeNode, ChangeRemoved, error->vidx()));
        }
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList DuplicateNodeCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("删除重复节点") << QStringLiteral("无");
    return methods;
}

Check::CheckType DuplicateNodeCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
