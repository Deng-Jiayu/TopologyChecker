#include "pointonlineendcheck.h"
#include "checkcontext.h"
#include "qgslinestring.h"
#include "featurepool.h"
#include "checkerror.h"

void PointOnLineEndCheck::collectErrors( const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids ) const
{
    Q_UNUSED( messages )

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds( featurePools ) : ids.toMap();
    QgsGeometryCheckerUtils::LayerFeatures layerFeatures( featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true );
    for ( const QgsGeometryCheckerUtils::LayerFeature &layerFeature : layerFeatures )
    {
        if (std::find(pointLayers.begin(), pointLayers.end(), layerFeature.layer()) == pointLayers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for ( int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart )
        {
            const QgsPoint *point = dynamic_cast<const QgsPoint *>( QgsGeometryCheckerUtils::getGeomPart( geom, iPart ) );
            if ( !point )
            {
                // Should not happen
                continue;
            }
            // Check that point lies on a line
            bool touches = false;
            QgsRectangle rect( point->x() - mContext->tolerance, point->y() - mContext->tolerance,
                              point->x() + mContext->tolerance, point->y() + mContext->tolerance );
            QgsGeometryCheckerUtils::LayerFeatures checkFeatures( featurePools, featureIds.keys(), rect, {QgsWkbTypes::LineGeometry}, mContext );
            for ( const QgsGeometryCheckerUtils::LayerFeature &checkFeature : checkFeatures )
            {
                if (std::find(lineLayers.begin(), lineLayers.end(), checkFeature.layer()) == lineLayers.end())
                    continue;
                const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();

                for ( int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart )
                {
                    const QgsLineString *testLine = dynamic_cast<const QgsLineString *>( QgsGeometryCheckerUtils::getGeomPart( testGeom, jPart ) );
                    if ( !testLine || testLine->numPoints() == 0)
                    {
                        continue;
                    }
                    if(point->distance(testLine->startPoint())<mContext->tolerance || point->distance(testLine->endPoint())<mContext->tolerance){
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

void PointOnLineEndCheck::fixError( const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & changes ) const
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

    // Check if polygon still exists
    if ( !vidx.isValid( geom ) )
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsPoint *point = dynamic_cast<const QgsPoint *>( QgsGeometryCheckerUtils::getGeomPart( geom, vidx.part ) );
    bool touches = false;
    QgsRectangle rect( point->x() - mContext->tolerance, point->y() - mContext->tolerance,
                      point->x() + mContext->tolerance, point->y() + mContext->tolerance );
    QMap<QString, QgsFeatureIds> featureIds = allLayerFeatureIds( featurePools );
    QgsGeometryCheckerUtils::LayerFeatures checkFeatures( featurePools, featureIds.keys(), rect, {QgsWkbTypes::LineGeometry}, mContext );
    for ( const QgsGeometryCheckerUtils::LayerFeature &checkFeature : checkFeatures )
    {
        if (std::find(lineLayers.begin(), lineLayers.end(), checkFeature.layer()) == lineLayers.end())
            continue;
        const QgsAbstractGeometry *testGeom = checkFeature.geometry().constGet();

        for ( int jPart = 0, mParts = testGeom->partCount(); jPart < mParts; ++jPart )
        {
            const QgsLineString *testLine = dynamic_cast<const QgsLineString *>( QgsGeometryCheckerUtils::getGeomPart( testGeom, jPart ) );
            if ( !testLine || testLine->numPoints() == 0)
            {
                continue;
            }
            if(point->distance(testLine->startPoint())<mContext->tolerance || point->distance(testLine->endPoint())<mContext->tolerance){
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

QStringList PointOnLineEndCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << tr( "Delete feature" ) << tr( "No action" );
    return methods;
}

Check::CheckType PointOnLineEndCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
