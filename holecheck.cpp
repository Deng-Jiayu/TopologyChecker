#include "checkcontext.h"
#include "holecheck.h"
#include "qgscurve.h"
#include "qgscurvepolygon.h"
#include "featurepool.h"
#include "checkerror.h"

void HoleCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            const QgsCurvePolygon *poly = dynamic_cast<const QgsCurvePolygon *>(CheckerUtils::getGeomPart(geom, iPart));
            if (!poly)
            {
                continue;
            }
            // Rings after the first one are interiors
            for (int iRing = 1, nRings = poly->ringCount(iPart); iRing < nRings; ++iRing)
            {

                QgsPoint pos = poly->interiorRing(iRing - 1)->centroid();
                errors.append(new CheckError(this, layerFeature, pos, QgsVertexId(iPart, iRing)));
            }
        }
    }
}

void HoleCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }
    QgsGeometry featureGeom = feature.geometry();
    const QgsAbstractGeometry *geom = featureGeom.constGet();
    QgsVertexId vidx = error->vidx();

    // Check if ring still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == RemoveHoles)
    {
        deleteFeatureGeometryRing(featurePools, error->layerId(), feature, vidx.part, vidx.ring, changes);
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList HoleCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("删除孔洞") << QStringLiteral("无");
    return methods;
}
