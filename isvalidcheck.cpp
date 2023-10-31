#include "isvalidcheck.h"
#include "checkcontext.h"
#include "featurepool.h"
#include "checkerror.h"
#include <qgsgeometryvalidator.h>

QList<QgsWkbTypes::GeometryType> IsValidCheck::compatibleGeometryTypes() const
{
    return factoryCompatibleGeometryTypes();
}

void IsValidCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, context());
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;

        QgsGeometry geometry = layerFeature.geometry();
        QVector<QgsGeometry::Error> geoErrors;

        if(GEOS)
            QgsGeometryValidator::validateGeometry(geometry, geoErrors, QgsGeometry::ValidatorGeos);
        else
            QgsGeometryValidator::validateGeometry(geometry, geoErrors, QgsGeometry::ValidatorQgisInternal);

        for ( const auto &error : qgis::as_const( geoErrors ) )
        {
            QgsGeometry errorGeometry;
            if ( error.hasWhere() )
                errorGeometry = QgsGeometry( qgis::make_unique<QgsPoint>( error.where() ) );

            errors.append(new CheckError(this,layerFeature.layerId(),layerFeature.feature().id(),errorGeometry,errorGeometry.asPoint(),QgsVertexId(),error.what()));
        }
    }
}

void IsValidCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }
    QgsGeometry geometry = feature.geometry();

    // Check if error still applies
    bool applies = false;
    QVector<QgsGeometry::Error> geoErrors;
    if(GEOS)
        QgsGeometryValidator::validateGeometry(geometry, geoErrors, QgsGeometry::ValidatorGeos);
    else
        QgsGeometryValidator::validateGeometry(geometry, geoErrors, QgsGeometry::ValidatorQgisInternal);

    for ( const auto &geoError : qgis::as_const( geoErrors ) )
    {
        if(geoError.what() == error->value() && geoError.where() == error->location())
        {
            applies = true;
            break;
        }
    }
    if(!applies)
    {
        error->setObsolete();
        return;
    }

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if(method == MakeValid)
    {
        QgsGeometry outputGeometry = feature.geometry().makeValid();
        if ( outputGeometry.isNull() || outputGeometry.equals(feature.geometry()) )
        {
            error->setFixFailed( QStringLiteral( "无法修复" ) );
            return;
        }

        if ( outputGeometry.wkbType() == QgsWkbTypes::Unknown ||
            QgsWkbTypes::flatType( outputGeometry.wkbType() ) == QgsWkbTypes::GeometryCollection )
        {
            // keep only the parts of the geometry collection with correct type
            const QVector< QgsGeometry > tmpGeometries = outputGeometry.asGeometryCollection();
            QVector< QgsGeometry > matchingParts;
            for ( const QgsGeometry &g : tmpGeometries )
            {
                if ( g.type() == feature.geometry().type() )
                    matchingParts << g;
            }
            if ( !matchingParts.empty() )
                outputGeometry = QgsGeometry::collectGeometry( matchingParts );
            else
                outputGeometry = QgsGeometry();
        }

        outputGeometry.convertToMultiType();
        if ( QgsWkbTypes::geometryType( outputGeometry.wkbType() ) != QgsWkbTypes::geometryType( feature.geometry().wkbType() ) )
        {
            // don't keep geometries which have different types - e.g. lines converted to points
            QString str = QStringLiteral("修复要素生成生成") + QgsWkbTypes::displayString( outputGeometry.wkbType() );
            error->setFixFailed( str );
        }
        else
        {
            FeaturePool *featurePool = featurePools[error->layerId()];

            feature.setGeometry( outputGeometry );
            changes[error->layerId()][feature.id()].append( Change( ChangeFeature, ChangeChanged ) );
            featurePool->updateFeature( feature );

            error->setFixed(method);
        }
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList IsValidCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("修复") << QStringLiteral("无");
    return methods;
}

QString IsValidCheck::id() const
{
    return factoryId();
}

QString IsValidCheck::factoryDescription()
{
    return QStringLiteral("无效几何");
}

QString IsValidCheck::description() const
{
    return factoryDescription();
}

Check::CheckType IsValidCheck::checkType() const
{
    return factoryCheckType();
}

QList<QgsWkbTypes::GeometryType> IsValidCheck::factoryCompatibleGeometryTypes()
{
    return {QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry};
}

bool IsValidCheck::factoryIsCompatible(QgsVectorLayer *layer)
{
    return factoryCompatibleGeometryTypes().contains(layer->geometryType());
}

QString IsValidCheck::factoryId()
{
    return QStringLiteral("IsValidCheck");
}

Check::CheckType IsValidCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}

CheckErrorSingle *IsValidCheck::convertToCheckError(SingleCheckError *error, const CheckerUtils::LayerFeature &layerFeature) const
{
    return new CheckErrorSingle(error, layerFeature);
}

IsValidCheckError::IsValidCheckError(const Check *check, const QgsGeometry &geometry, const QgsGeometry &errorLocation, const QString &errorDescription)
    : SingleCheckError(check, geometry, errorLocation), mDescription(errorDescription)
{
}

QString IsValidCheckError::description() const
{
    return mDescription;
}
