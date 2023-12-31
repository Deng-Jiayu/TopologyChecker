﻿#include "pointonlinenodecheck.h"
#include "checkcontext.h"
#include "qgslinestring.h"
#include "featurepool.h"
#include "checkerror.h"

void PointOnLineNodeCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

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
            // Check that point lies on a line node
            bool touches = false;
            QgsRectangle rect(point->x() - mContext->tolerance, point->y() - mContext->tolerance,
                              point->x() + mContext->tolerance, point->y() + mContext->tolerance);
            CheckerUtils::LayerFeatures checkFeatures(featurePools, featureIds.keys(), rect, {QgsWkbTypes::LineGeometry}, mContext);
            for (const CheckerUtils::LayerFeature &checkFeature : checkFeatures)
            {
                if (std::find(lineLayers.begin(), lineLayers.end(), checkFeature.layer()) == lineLayers.end())
                    continue;
                const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
                for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
                {
                    const QgsLineString *testLine = dynamic_cast<const QgsLineString *>(CheckerUtils::getGeomPart(testGeom, jPart));
                    if (!testLine || testLine->numPoints() <= 2)
                    {
                        continue;
                    }
                    for (int i = 1; i < testLine->numPoints() - 1; ++i)
                    {
                        if (point->distance(testLine->pointN(i)) < mContext->tolerance)
                        {
                            touches = true;
                            break;
                        }
                    }
                    if (touches)
                        break;
                }
                if (touches)
                {
                    break;
                }
            }
            if (touches)
            {
                continue;
            }
            errors.append(new CheckError(this, layerFeature, *point, QgsVertexId(iPart, 0, 0)));
        }
    }
}

void PointOnLineNodeCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
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

    // Check if point still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsPoint *point = dynamic_cast<const QgsPoint *>(CheckerUtils::getGeomPart(geom, vidx.part));
    bool touches = false;
    QgsRectangle rect(point->x() - mContext->tolerance, point->y() - mContext->tolerance,
                      point->x() + mContext->tolerance, point->y() + mContext->tolerance);
    QMap<QString, QgsFeatureIds> featureIds = allLayerFeatureIds(featurePools);
    CheckerUtils::LayerFeatures checkFeatures(featurePools, featureIds.keys(), rect, {QgsWkbTypes::LineGeometry}, mContext);
    for (const CheckerUtils::LayerFeature &checkFeature : checkFeatures)
    {
        if (std::find(lineLayers.begin(), lineLayers.end(), checkFeature.layer()) == lineLayers.end())
            continue;
        const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
        for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
        {
            const QgsLineString *testLine = dynamic_cast<const QgsLineString *>(CheckerUtils::getGeomPart(testGeom, jPart));
            if (!testLine || testLine->numPoints() <= 2)
            {
                continue;
            }
            for (int i = 1; i < testLine->numPoints() - 1; ++i)
            {
                if (point->distance(testLine->pointN(i)) < mContext->tolerance)
                {
                    touches = true;
                    break;
                }
            }
            if (touches)
                break;
        }
        if (touches)
            break;
    }
    if (touches)
    {
        error->setObsolete();
        return;
    }

    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == Delete)
    {
        deleteFeatureGeometryPart(featurePools, error->layerId(), feature, vidx.part, changes);
        error->setFixed(method);
    }
    else if (method == Snap)
    {
        QgsPoint *ans = new QgsPoint;
        bool init = true;
        for (const CheckerUtils::LayerFeature &checkFeature : checkFeatures)
        {
            if (std::find(lineLayers.begin(), lineLayers.end(), checkFeature.layer()) == lineLayers.end())
                continue;
            const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
            for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
            {
                const QgsLineString *testLine = dynamic_cast<const QgsLineString *>(CheckerUtils::getGeomPart(testGeom, jPart));
                if (!testLine || testLine->numPoints() <= 2)
                {
                    continue;
                }
                if (init)
                {
                    ans->setX(testLine->pointN(1).x());
                    ans->setY(testLine->pointN(1).y());
                    init = false;
                }
                for (int i = 1; i < testLine->numPoints() - 1; ++i)
                {
                    if (point->distance(testLine->pointN(i)) < point->distance(*ans))
                    {
                        ans->setX(testLine->pointN(i).x());
                        ans->setY(testLine->pointN(i).y());
                    }
                }
            }
        }
        if (ans->asWkt(3) == "Point EMPTY")
        {
            error->setFixFailed(QStringLiteral("容差范围内无线"));
            return;
        }

        // avoid intersections
        QgsGeometry *newGeom = new QgsGeometry(ans);

        QString layerId = error->layerId();
        int partIdx = vidx.part;

        replaceFeatureGeometryPart(featurePools, layerId, feature, partIdx, newGeom->get(), changes);
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList PointOnLineNodeCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("吸附至节点") << QStringLiteral("删除点要素") << QStringLiteral("无");
    return methods;
}

Check::CheckType PointOnLineNodeCheck::factoryCheckType()
{
    return Check::LayerCheck;
}
