#include "attrvalidcheck.h"
#include "checkcontext.h"
#include "featurepool.h"
#include "checkerror.h"

void AttrValidCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;

        QgsFeature f = layerFeature.feature();
        QgsFields pFields = f.fields();
        if(f.attribute(attr).isValid() && f.attribute(attr).isNull()){
            errors.append(new CheckError(this, layerFeature, layerFeature.geometry().centroid().asPoint(), QgsVertexId(), attr));
        }
    }
}

void AttrValidCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    QgsFields pFields = feature.fields();
    if( !(feature.attribute(attr).isValid() && feature.attribute(attr).isNull())){
        error->setObsolete();
        return;
    }

    // Fix with selected method
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList AttrValidCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("无");
    return methods;
}
