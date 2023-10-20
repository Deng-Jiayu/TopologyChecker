#include "pointonlinecheck.h"

#include "checkcontext.h"
#include "pointonlinecheck.h"
#include "qgslinestring.h"
#include "featurepool.h"
#include "checkerror.h"

void PointOnLineCheck::collectErrors( const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids ) const
{
    Q_UNUSED( messages )

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds( featurePools ) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures( featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true );
    for ( const CheckerUtils::LayerFeature &layerFeature : layerFeatures )
    {
        if (std::find(pointLayers.begin(), pointLayers.end(), layerFeature.layer()) == pointLayers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for ( int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart )
        {
            const QgsPoint *point = dynamic_cast<const QgsPoint *>( CheckerUtils::getGeomPart( geom, iPart ) );
            if ( !point )
            {
                // Should not happen
                continue;
            }
            // Check that point lies on a line
            bool touches = false;
            QgsRectangle rect( point->x() - mContext->tolerance, point->y() - mContext->tolerance,
                              point->x() + mContext->tolerance, point->y() + mContext->tolerance );
            CheckerUtils::LayerFeatures checkFeatures( featurePools, featureIds.keys(), rect, {QgsWkbTypes::LineGeometry}, mContext );
            for ( const CheckerUtils::LayerFeature &checkFeature : checkFeatures )
            {
                if (std::find(lineLayers.begin(), lineLayers.end(), checkFeature.layer()) == lineLayers.end())
                    continue;
                const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
                for ( int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart )
                {
                    const QgsLineString *testLine = dynamic_cast<const QgsLineString *>( CheckerUtils::getGeomPart( testGeom, jPart ) );
                    if ( !testLine )
                    {
                        continue;
                    }
                    if ( CheckerUtils::pointOnLine( *point, testLine, mContext->tolerance ) )
                    {
                        touches = true;
                        break;
                    }
                }
                if ( touches )
                {
                    break;
                }
            }
            if ( touches )
            {
                continue;
            }
            errors.append( new CheckError( this, layerFeature, *point, QgsVertexId( iPart, 0, 0 ) ) );
        }
    }
}

void PointOnLineCheck::fixError( const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & changes ) const
{
    FeaturePool *featurePool = featurePools[ error->layerId() ];
    QgsFeature feature;
    if ( !featurePool->getFeature( error->featureId(), feature ) )
    {
        error->setObsolete();
        return;
    }
    const QgsGeometry g = feature.geometry();
    const QgsAbstractGeometry *geom = g.constGet();
    QgsVertexId vidx = error->vidx();

    // Check if point still exists
    if ( !vidx.isValid( geom ) )
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsPoint *point = dynamic_cast<const QgsPoint *>( CheckerUtils::getGeomPart( geom, vidx.part ) );
    bool touches = false;
    QgsRectangle rect( point->x() - mContext->tolerance, point->y() - mContext->tolerance,
                      point->x() + mContext->tolerance, point->y() + mContext->tolerance );
    QMap<QString, QgsFeatureIds> featureIds = allLayerFeatureIds( featurePools );
    CheckerUtils::LayerFeatures checkFeatures( featurePools, featureIds.keys(), rect, {QgsWkbTypes::LineGeometry}, mContext );
    for ( const CheckerUtils::LayerFeature &checkFeature : checkFeatures )
    {
        if (std::find(lineLayers.begin(), lineLayers.end(), checkFeature.layer()) == lineLayers.end())
            continue;
        const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();
        for ( int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart )
        {
            const QgsLineString *testLine = dynamic_cast<const QgsLineString *>( CheckerUtils::getGeomPart( testGeom, jPart ) );
            if ( !testLine )
            {
                continue;
            }
            if ( CheckerUtils::pointOnLine( *point, testLine, mContext->tolerance ) )
            {
                touches = true;
                break;
            }
        }
        if ( touches ) break;
    }
    if ( touches )
    {
        error->setObsolete();
        return;
    }

    // Fix with selected method
    if ( method == NoChange )
    {
        error->setFixed( method );
    }
    else if ( method == Delete )
    {
        deleteFeatureGeometryPart( featurePools, error->layerId(), feature, vidx.part, changes );
        error->setFixed( method );
    }
    else
    {
        error->setFixFailed( tr( "Unknown method" ) );
    }
}

QStringList PointOnLineCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << tr( "Delete feature" ) << tr( "No action" );
    return methods;
}

Check::CheckType PointOnLineCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
