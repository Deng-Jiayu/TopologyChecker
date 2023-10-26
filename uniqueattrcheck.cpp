#include "uniqueattrcheck.h"
#include "checkcontext.h"
#include "featurepool.h"
#include "checkerror.h"

void UniqueAttrCheck::collectErrors( const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids ) const
{
    Q_UNUSED( messages )

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds( featurePools ) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures( featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext );

    for ( const CheckerUtils::LayerFeature &layerFeatureA : layerFeatures )
    {
        QString attr = "id";
        QgsFeature f = layerFeatureA.feature();

        if(f.attribute(attr).isValid() && f.attribute(attr).isNull()){
            errors.append(new CheckError(this, layerFeatureA, layerFeatureA.geometry().centroid().asPoint(), QgsVertexId(), attr + QStringLiteral("缺失")));
            continue;
        }

        QVariant var = f.attribute(attr);
        QList<QgsFeatureId> duplicates;

        QgsGeometry geomA = layerFeatureA.geometry();
        QgsRectangle bboxA = geomA.boundingBox();
        QgsWkbTypes::GeometryType geomType = geomA.type();
        CheckerUtils::LayerFeatures layerFeaturesB( featurePools, QList<QString>()<<layerFeatureA.layer()->id(), bboxA, {geomType}, mContext );
        for ( const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB )
        {
            if ( layerFeatureB.feature().id() >= layerFeatureA.feature().id() )
            {
                continue;
            }




        }
        if ( !duplicates.isEmpty() )
        {
            errors.append( new AttrDuplicateError( this, layerFeatureA, geomA.constGet()->centroid(), featurePools, duplicates ) );
        }
    }
}

void UniqueAttrCheck::fixError( const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes ) const
{
    FeaturePool *featurePool = featurePools[ error->layerId() ];
    QgsFeature feature;
    if ( !featurePool->getFeature( error->featureId(), feature ) )
    {
        error->setObsolete();
        return;
    }


    // Fix error
    if ( method == NoChange )
    {
        error->setFixed( method );
    }
    else
    {
        error->setFixFailed( tr( "Unknown method" ) );
    }
}

QStringList UniqueAttrCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral( "无" );
    return methods;
}
