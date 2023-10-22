#include "danglecheck.h"
#include "checkcontext.h"
#include "qgslinestring.h"
#include "qgsvectorlayer.h"
#include "checkerror.h"

void DangleCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
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
            const QgsLineString *line = dynamic_cast<const QgsLineString *>(CheckerUtils::getGeomPart(geom, iPart));
            if (!line)
            {
                // Should not happen
                continue;
            }
            // Check that start and end node lie on a line
            int nVerts = geom->vertexCount(iPart, 0);
            const QgsPoint &p1 = geom->vertexAt(QgsVertexId(iPart, 0, 0));
            const QgsPoint &p2 = geom->vertexAt(QgsVertexId(iPart, 0, nVerts - 1));

            bool p1touches = CheckerUtils::pointOnLine(p1, line, mContext->tolerance, true);
            bool p2touches = CheckerUtils::pointOnLine(p2, line, mContext->tolerance, true);

            if (p1touches && p2touches)
            {
                // Both endpoints lie on line itself
                continue;
            }

            // Check whether endpoints line on another line in the layer
            CheckerUtils::LayerFeatures checkFeatures(featurePools, QList<QString>() << layerFeature.layer()->id(), line->boundingBox(), {QgsWkbTypes::LineGeometry}, mContext);
            for (const CheckerUtils::LayerFeature &checkFeature : checkFeatures)
            {
                if (std::find(layers.begin(), layers.end(), checkFeature.layer()) == layers.end())
                    continue;
                const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
                for (int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart)
                {
                    if (checkFeature.feature().id() == layerFeature.feature().id() && iPart == jPart)
                    {
                        // Skip current feature part, it was already checked above
                        continue;
                    }
                    const QgsLineString *testLine = dynamic_cast<const QgsLineString *>(CheckerUtils::getGeomPart(testGeom, jPart));
                    if (!testLine)
                    {
                        continue;
                    }
                    p1touches = p1touches || CheckerUtils::pointOnLine(p1, testLine, mContext->tolerance);
                    p2touches = p2touches || CheckerUtils::pointOnLine(p2, testLine, mContext->tolerance);
                    if (p1touches && p2touches)
                    {
                        break;
                    }
                }
                if (p1touches && p2touches)
                {
                    break;
                }
            }
            if (p1touches && p2touches)
            {
                continue;
            }
            if (!p1touches)
            {
                errors.append(new CheckError(this, layerFeature, p1, QgsVertexId(iPart, 0, 0)));
            }
            if (!p2touches)
            {
                errors.append(new CheckError(this, layerFeature, p2, QgsVertexId(iPart, 0, nVerts - 1)));
            }
        }
    }
}

void DangleCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & /*changes*/) const
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

QStringList DangleCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
    return methods;
}

Check::CheckType DangleCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
