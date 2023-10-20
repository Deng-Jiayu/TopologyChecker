#include "pointonboundarycheck.h"
#include "checkcontext.h"
#include "checkerror.h"

void PointOnBoundaryCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            const QgsPoint *point = dynamic_cast<const QgsPoint *>(CheckerUtils::getGeomPart(geom, iPart));
            if (!point)
            {
                // Should not happen
                continue;
            }
            QgsPoint q(point->x(), point->y());
            // Check that point lies on boundaries
            bool touches = false;
            QgsRectangle rect(point->x() - mContext->tolerance, point->y() - mContext->tolerance,
                              point->x() + mContext->tolerance, point->y() + mContext->tolerance);
            CheckerUtils::LayerFeatures checkFeatures(featurePools, featureIds.keys(), rect, {QgsWkbTypes::PolygonGeometry}, mContext);
            for (const CheckerUtils::LayerFeature &checkFeature : checkFeatures)
            {
                const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
                for ( int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart )
                {
                    auto it1 = testGeom[jPart].vertices_begin();
                    auto it2 = testGeom[jPart].vertices_begin();
                    ++it2;
                    while(it2 != testGeom[jPart].vertices_end())
                    {
                        QgsPoint p1 = *it1;
                        QgsPoint p2 = *it2;
                        double nom = std::fabs( ( p2.y() - p1.y() ) * q.x() - ( p2.x() - p1.x() ) * q.y() + p2.x() * p1.y() - p2.y() * p1.x() );
                        double dx = p2.x() - p1.x();
                        double dy = p2.y() - p1.y();
                        if( (nom / std::sqrt( dx * dx + dy * dy ) ) < mContext->tolerance){
                            touches = true;
                            break;
                        }
                        ++it1; ++it2;
                    }
                    if( touches ) break;
                }
                if ( touches ) break;
            }
            if ( touches ) continue;
            errors.append( new CheckError( this, layerFeature, *point, QgsVertexId( iPart, 0, 0 ) ) );
        }
    }
}

void PointOnBoundaryCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & /*changes*/) const
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

QStringList PointOnBoundaryCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << tr("No action");
    return methods;
}

Check::CheckType PointOnBoundaryCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
