#include "convexhullcheck.h"
#include "checkerror.h"
#include "checkerutils.h"
#include "qgsgeometryengine.h"

bool ConvexHullCheck::reverse = false;

void ConvexHullCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        std::unique_ptr<QgsGeometryEngine> geomEngine = CheckerUtils::createGeomEngine(geom, mContext->tolerance);
        if (!geomEngine->isValid())
        {
            messages.append(tr("ConvexHull check failed for (%1): the geometry is invalid").arg(layerFeature.id()));
            continue;
        }
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            // TODO, use QgsGeometryEngine
            const QgsAbstractGeometry *part = CheckerUtils::getGeomPart(geom, iPart);
            QString wkt = part->asWkt();
            QgsGeometry g = QgsGeometry::fromWkt(wkt);
            QgsGeometry convexhull = g.convexHull();
            QgsGeometry buffer = g.buffer(mContext->tolerance, 5);
            if ( (!reverse && !buffer.contains(convexhull)) || (reverse && buffer.contains(convexhull)) ) {
                errors.append(new CheckError(this, layerFeature, part->centroid(), QgsVertexId(iPart)));
            }
        }
    }
}


void ConvexHullCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }

    QgsGeometry g = feature.geometry();
    const QgsAbstractGeometry *geom = g.constGet();
    QgsVertexId vidx = error->vidx();

    // Check if polygon still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsAbstractGeometry *part = CheckerUtils::getGeomPart(geom, vidx.part);
    QString wkt = part->asWkt();
    QgsGeometry tempGeo = QgsGeometry::fromWkt(wkt);
    QgsGeometry convexhull = tempGeo.convexHull();
    QgsGeometry buffer = tempGeo.buffer(mContext->tolerance, 5);
    if( !reverse && buffer.contains(convexhull) )
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
    else if (method == TransformToConvexHull)
    {
        if(reverse)
        {
            error->setFixFailed(QStringLiteral("已是凸包"));
            return;
        }
        QString errMsg;
        if (transformToConvexHull(featurePools, error->layerId(), feature, vidx.part, method, mergeAttributeIndices[error->layerId()], changes, errMsg))
        {
            error->setFixed(method);
        }
        else
        {
            error->setFixFailed(QStringLiteral("转化失败"));
        }
    }
    else
    {
        error->setFixFailed(("Unknown method"));
    }
}

bool ConvexHullCheck::transformToConvexHull(const QMap<QString, FeaturePool *> &featurePools, const QString &layerId, QgsFeature &feature, int partIdx, int method, int mergeAttributeIndex, Changes &changes, QString &errMsg) const
{
    QgsGeometry featureGeometry = feature.geometry();
    const QgsAbstractGeometry *geom = featureGeometry.constGet();
    std::unique_ptr<QgsGeometryEngine> geomEngine(CheckerUtils::createGeomEngine(CheckerUtils::getGeomPart(geom, partIdx), mContext->reducedTolerance));

    QgsAbstractGeometry *newGeom = geomEngine->convexHull();
    replaceFeatureGeometryPart(featurePools, layerId, feature, partIdx, newGeom, changes);
    return true;
}

QStringList ConvexHullCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("转化为凸包")
                                 << QStringLiteral("删除要素")
                                 << QStringLiteral("无");
    return methods;
}
