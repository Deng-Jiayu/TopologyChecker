#include "polygoncoveredbypolygoncheck.h"
#include "checkcontext.h"
#include "qgsfeedback.h"
#include "qgsgeometry.h"
#include "featurepool.h"
#include "qgsvectorlayer.h"
#include "checkerror.h"

void PolygonCoveredByPolygonCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    for (const CheckerUtils::LayerFeature &layerFeatureA : layerFeaturesA)
    {
        if (std::find(layersA.begin(), layersA.end(), layerFeatureA.layer()) == layersA.end())
            continue;
        if (feedback && feedback->isCanceled())
            break;

        QVector<QgsGeometry> geomsA = layerFeatureA.geometry().asGeometryCollection();
        for (int iPart = 0; iPart < geomsA.size(); ++iPart)
        {
            QgsGeometry &polygon = geomsA[iPart];

            QgsGeometry boundarys;
            featureIds = allLayerFeatureIds(featurePools);
            CheckerUtils::LayerFeatures layerFeaturesB(featurePools, featureIds.keys(), polygon.boundingBox(), {QgsWkbTypes::PolygonGeometry}, mContext);
            for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
            {
                if (std::find(layersB.begin(), layersB.end(), layerFeatureB.layer()) == layersB.end())
                    continue;
                QVector<QgsGeometry> geomsB = layerFeatureB.geometry().asGeometryCollection();

                for (int jPart = 0; jPart < geomsB.size(); ++jPart)
                {
                    QgsGeometry &boundary = geomsB[jPart];
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
            if (!polygon.within(boundarys))
            {
                errors.append(new CheckError(this, layerFeatureA, layerFeatureA.geometry().centroid().asPoint(), QgsVertexId(iPart)));
            }
        }
    }
}

void PolygonCoveredByPolygonCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePoolA = featurePools[error->layerId()];
    QgsFeature featureA;
    if (!featurePoolA->getFeature(error->featureId(), featureA))
    {
        error->setObsolete();
        return;
    }

    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList PolygonCoveredByPolygonCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("无");
    return methods;
}

QString PolygonCoveredByPolygonCheck::factoryId()
{
    return QStringLiteral("PolygonCoveredByPolygonCheck");
}

Check::CheckType PolygonCoveredByPolygonCheck::factoryCheckType()
{
    return Check::FeatureCheck;
}
