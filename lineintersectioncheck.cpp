#include "lineintersectioncheck.h"
#include "checkcontext.h"
#include "qgslinestring.h"
#include "qgsvectorlayer.h"
#include "checkerror.h"

void LineIntersectionCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    QList<QString> layerIds = featureIds.keys();
    for (const CheckerUtils::LayerFeature &layerFeatureA : layerFeaturesA)
    {
        if (std::find(layers.begin(), layers.end(), layerFeatureA.layer()) == layers.end())
            continue;
        // Ensure each pair of layers only gets compared once: remove the current layer from the layerIds, but add it to the layerList for layerFeaturesB
        layerIds.removeOne(layerFeatureA.layer()->id());

        const QgsAbstractGeometry *geom = layerFeatureA.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            const QgsLineString *line = dynamic_cast<const QgsLineString *>(CheckerUtils::getGeomPart(geom, iPart));
            if (!line)
            {
                // Should not happen
                continue;
            }

            // Check whether the line intersects with any other lines
            CheckerUtils::LayerFeatures layerFeaturesB(featurePools, QList<QString>() << layerFeatureA.layer()->id() << layerIds, line->boundingBox(), {QgsWkbTypes::LineGeometry}, mContext);
            for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
            {
                if(layerFeatureA.layer()->id() != layerFeatureB.layer()->id()) continue;
                // > : only report intersections within same layer once
                if (layerFeatureA.layer()->id() == layerFeatureB.layer()->id() && layerFeatureB.feature().id() > layerFeatureA.feature().id())
                {
                    continue;
                }

                const QgsAbstractGeometry *testGeom = layerFeatureB.geometry().constGet();
                for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
                {
                    // Skip current feature part, only report intersections within same part once
                    if (layerFeatureB.feature().id() == layerFeatureA.feature().id() && iPart >= jPart)
                    {
                        continue;
                    }
                    const QgsLineString *testLine = dynamic_cast<const QgsLineString *>(CheckerUtils::getGeomPart(testGeom, jPart));
                    if (!testLine)
                    {
                        continue;
                    }
                    const QList<QgsPoint> intersections = CheckerUtils::lineIntersections(line, testLine, mContext->tolerance, !excludeEndpoint);
                    for (const QgsPoint &inter : intersections)
                    {
                        errors.append(new CheckError(this, layerFeatureA, inter, QgsVertexId(iPart), layerFeatureB.id()));
                    }
                }
            }
        }
    }
}

void LineIntersectionCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & /*changes*/) const
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

QStringList LineIntersectionCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
    return methods;
}

Check::CheckType LineIntersectionCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
