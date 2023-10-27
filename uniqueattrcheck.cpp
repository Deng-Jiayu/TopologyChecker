#include "uniqueattrcheck.h"
#include "checkcontext.h"
#include "featurepool.h"
#include "checkerror.h"

void UniqueAttrCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);

    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        QgsFeature f = layerFeature.feature();
        if (!f.attribute(attr).isValid())
            continue;

        if (f.attribute(attr).isNull())
        {
            errors.append(new CheckError(this, layerFeature, layerFeature.geometry().centroid().asPoint(), QgsVertexId(), attr + QStringLiteral("属性缺失")));
            continue;
        }

        QVariant var = f.attribute(attr);
        QList<QgsFeatureId> duplicates;

        QgsVectorLayer *layer = layerFeature.layer();
        QgsFeatureIterator featureIt = layer->getFeatures();
        featureIt.rewind();
        QgsFeature checkF;
        while (featureIt.nextFeature(checkF))
        {
            if (checkF.id() >= f.id())
                continue;
            if (checkF.attribute(attr) == var)
                duplicates.append(checkF.id());
        }

        if (!duplicates.isEmpty())
        {
            errors.append(new AttrDuplicateError(this, layerFeature, layerFeature.geometry().centroid().asPoint(), attr, duplicates));
        }
    }
}

void UniqueAttrCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }

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

QStringList UniqueAttrCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
    return methods;
}

QString AttrDuplicateError::duplicatesString(const QString &attr, const QList<QgsFeatureId> &duplicates)
{

    QString str = attr + QStringLiteral("属性重复：");
    for (auto it = duplicates.constBegin(); it != duplicates.constEnd(); ++it)
    {
        QString ids;
        ids.append(QString::number((*it)));
        str += ids + ',';
    }
    str.remove(str.length() - 1, 1);
    return str + "; ";
}
