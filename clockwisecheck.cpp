#include "clockwisecheck.h"
#include "checkcontext.h"
#include "featurepool.h"
#include "checkerror.h"
#include "qgslinestring.h"

bool ClockwiseCheck::reverse = false;

QList<QgsWkbTypes::GeometryType> ClockwiseCheck::compatibleGeometryTypes() const
{
    return factoryCompatibleGeometryTypes();
}

void ClockwiseCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, context());
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            const QgsAbstractGeometry *part = CheckerUtils::getGeomPart(geom, iPart);
            const QgsLineString *line = dynamic_cast<const QgsLineString *>(part);
            if (!line)
            {
                QgsLineString l;
                for(auto it  = part->vertices_begin(); it != part->vertices_end(); ++it){
                    l.addVertex(*it);
                }
                line = &l;
            }
            if( (line->orientation() == QgsLineString::Clockwise && reverse)
                || (line->orientation() == QgsLineString::CounterClockwise && !reverse) ){
                errors.append(new CheckError(this, layerFeature, part->centroid(), QgsVertexId(iPart)));
            }
        }
    }
}

void ClockwiseCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
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

QStringList ClockwiseCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
    return methods;
}

QString ClockwiseCheck::id() const
{
    return factoryId();
}

QString ClockwiseCheck::factoryDescription()
{
    if (!reverse)
        return QStringLiteral("逆时针");
    else
        return QStringLiteral("顺时针");
}

QString ClockwiseCheck::description() const
{
    return factoryDescription();
}

Check::CheckType ClockwiseCheck::checkType() const
{
    return factoryCheckType();
}

QList<QgsWkbTypes::GeometryType> ClockwiseCheck::factoryCompatibleGeometryTypes()
{
    return {QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry};
}

bool ClockwiseCheck::factoryIsCompatible(QgsVectorLayer *layer)
{
    return factoryCompatibleGeometryTypes().contains(layer->geometryType());
}

QString ClockwiseCheck::factoryId()
{
    return QStringLiteral("ClockwiseCheck");
}

Check::CheckType ClockwiseCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
