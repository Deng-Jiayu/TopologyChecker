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
        QVector<QgsGeometry::Error> geometryErrors;

        QgsGeometry::ValidationMethod method = QgsGeometry::ValidatorQgisInternal;
        if (QgsSettings().value(QStringLiteral("qgis/digitizing/validate_geometries"), 1).toInt() == 2)
            method = QgsGeometry::ValidatorGeos;

        QgsGeometryValidator validator(geometry, &geometryErrors, method);

        QObject::connect(&validator, &QgsGeometryValidator::errorFound, &validator, [&geometryErrors](const QgsGeometry::Error &error)
                         { geometryErrors.append(error); });

        // We are already on a thread here normally, no reason to start yet another one. Run synchronously.
        validator.run();

        for (const auto &error : qgis::as_const(geometryErrors))
        {
            QgsGeometry errorGeometry;
            if (error.hasWhere())
                errorGeometry = QgsGeometry(qgis::make_unique<QgsPoint>(error.where()));

            errors.append( new CheckError(this, layerFeature.layerId(), layerFeature.feature().id(), errorGeometry, errorGeometry.centroid().asPoint()));
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
    QgsGeometry featureGeometry = feature.geometry();
    QgsAbstractGeometry *geometry = featureGeometry.get();
    QgsVertexId vidx = error->vidx();

    // Check if point still exists
    if (!vidx.isValid(geometry))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList IsValidCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
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
