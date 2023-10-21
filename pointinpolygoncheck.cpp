#include "pointinpolygoncheck.h"
#include "checkcontext.h"
#include "qgsgeometryengine.h"
#include "checkerror.h"

void PointInPolygonCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(pointLayers.begin(), pointLayers.end(), layerFeature.layer()) == pointLayers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            const QgsPoint *point = dynamic_cast<const QgsPoint *>(CheckerUtils::getGeomPart(geom, iPart));
            if (!point)
            {
                // Should not happen
                continue;
            }
            int nTested = 0;
            int nInside = 0;

            // Check whether point is contained by a fully contained by a polygon
            QgsRectangle rect(point->x() - mContext->tolerance, point->y() - mContext->tolerance,
                              point->x() + mContext->tolerance, point->y() + mContext->tolerance);
            CheckerUtils::LayerFeatures checkFeatures(featurePools, featureIds.keys(), rect, {QgsWkbTypes::PolygonGeometry}, mContext);
            for (const CheckerUtils::LayerFeature &checkFeature : checkFeatures)
            {
                if (std::find(polygonLayers.begin(), polygonLayers.end(), checkFeature.layer()) == polygonLayers.end())
                    continue;
                ++nTested;
                const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
                std::unique_ptr<QgsGeometryEngine> testGeomEngine = CheckerUtils::createGeomEngine(testGeom, mContext->reducedTolerance);
                if (!testGeomEngine->isValid())
                {
                    messages.append(tr("Point in polygon check failed for (%1): the geometry is invalid").arg(checkFeature.id()));
                    continue;
                }
                if (testGeomEngine->contains(point) && !testGeomEngine->touches(point))
                {
                    ++nInside;
                }
            }
            if (nTested == 0 || nTested != nInside)
            {
                errors.append(new CheckError(this, layerFeature, *point, QgsVertexId(iPart, 0, 0)));
            }
        }
    }
}

void PointInPolygonCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & /*changes*/) const
{
    Q_UNUSED(featurePools)

    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList PointInPolygonCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
    return methods;
}

Check::CheckType PointInPolygonCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
