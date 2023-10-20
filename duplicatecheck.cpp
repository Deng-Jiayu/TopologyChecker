#include "duplicatecheck.h"
#include "checkcontext.h"
#include "qgsgeometryengine.h"
#include "qgsspatialindex.h"
#include "qgsgeometry.h"
#include "featurepool.h"
#include "qgsvectorlayer.h"

QString DuplicateCheckError::duplicatesString(const QMap<QString, FeaturePool *> &featurePools, const QMap<QString, QList<QgsFeatureId>> &duplicates)
{
    QStringList str;
    for (auto it = duplicates.constBegin(); it != duplicates.constEnd(); ++it)
    {
        str.append(featurePools[it.key()]->layer()->name() + ":");
        QStringList ids;
        ids.reserve(it.value().length());
        for (QgsFeatureId id : it.value())
        {
            ids.append(QString::number(id));
        }
        str.back() += ids.join(',');
    }
    return str.join(QLatin1String("; "));
}

void DuplicateCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, types, feedback, mContext, true);
    QList<QString> layerIds = featureIds.keys();
    for (const CheckerUtils::LayerFeature &layerFeatureA : layerFeaturesA)
    {
        if (std::find(layers.begin(), layers.end(), layerFeatureA.layer()) == layers.end())
            continue;
        // Ensure each pair of layers only gets compared once: remove the current layer from the layerIds, but add it to the layerList for layerFeaturesB
        layerIds.removeOne(layerFeatureA.layer()->id());

        QgsGeometry geomA = layerFeatureA.geometry();
        QgsRectangle bboxA = geomA.boundingBox();
        std::unique_ptr<QgsGeometryEngine> geomEngineA = CheckerUtils::createGeomEngine(geomA.constGet(), mContext->tolerance);
        if (!geomEngineA->isValid())
        {
            messages.append(tr("Duplicate check failed for (%1): the geometry is invalid").arg(layerFeatureA.id()));
            continue;
        }
        QMap<QString, QList<QgsFeatureId>> duplicates;

        QgsWkbTypes::GeometryType geomType = geomA.type();
        CheckerUtils::LayerFeatures layerFeaturesB(featurePools, QList<QString>() << layerFeatureA.layer()->id() << layerIds, bboxA, {geomType}, mContext);
        for (const CheckerUtils::LayerFeature &layerFeatureB : layerFeaturesB)
        {
            if (std::find(layers.begin(), layers.end(), layerFeatureB.layer()) == layers.end())
                continue;
            // > : only report overlaps within same layer once
            if (layerFeatureA.layer()->id() == layerFeatureB.layer()->id() && layerFeatureB.feature().id() >= layerFeatureA.feature().id())
            {
                continue;
            }

            const QgsGeometry geomB = layerFeatureB.geometry();
            QString errMsg;
            const bool equal = geomEngineA->isEqual( geomB.constGet(), &errMsg );
            if (equal && errMsg.isEmpty())
            {
                duplicates[layerFeatureB.layer()->id()].append(layerFeatureB.feature().id());
            }
            else if (!errMsg.isEmpty())
            {
                messages.append(tr("Duplicate check failed for (%1, %2): %3").arg(layerFeatureA.id(), layerFeatureB.id(), errMsg));
            }
        }
        if (!duplicates.isEmpty())
        {
            errors.append(new DuplicateCheckError(this, layerFeatureA, geomA.constGet()->centroid(), featurePools, duplicates));
        }
    }
}

void DuplicateCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePoolA = featurePools[error->layerId()];
    QgsFeature featureA;
    if (!featurePoolA->getFeature(error->featureId(), featureA))
    {
        error->setObsolete();
        return;
    }

    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == RemoveDuplicates)
    {
        CheckerUtils::LayerFeature layerFeatureA(featurePoolA, featureA, mContext, true);
        std::unique_ptr<QgsGeometryEngine> geomEngineA = CheckerUtils::createGeomEngine(layerFeatureA.geometry().constGet(), mContext->tolerance);

        DuplicateCheckError *duplicateError = static_cast<DuplicateCheckError *>(error);
        const QMap<QString, QList<QgsFeatureId>> duplicates = duplicateError->duplicates();
        for (auto it = duplicates.constBegin(); it != duplicates.constEnd(); ++it)
        {
            const QString layerIdB = it.key();
            FeaturePool *featurePoolB = featurePools[layerIdB];
            const QList<QgsFeatureId> ids = it.value();
            for (QgsFeatureId idB : ids)
            {
                QgsFeature featureB;
                if (!featurePoolB->getFeature(idB, featureB))
                {
                    continue;
                }

                CheckerUtils::LayerFeature layerFeatureB(featurePoolB, featureB, mContext, true);
                const QgsGeometry geomB = layerFeatureB.geometry();
                QString errMsg;
                const bool equal = geomEngineA->isEqual( geomB.constGet(), &errMsg );
                if (equal && errMsg.isEmpty())
                {
                    featurePoolB->deleteFeature(featureB.id());
                    changes[layerIdB][idB].append(Change(ChangeFeature, ChangeRemoved));
                }
            }
        }
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList DuplicateCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("无")
                                 << QStringLiteral("移除重复");
    return methods;
}

QString DuplicateCheck::factoryId()
{
    return QStringLiteral("DuplicateCheck");
}

Check::CheckType DuplicateCheck::factoryCheckType()
{
    return Check::FeatureCheck;
}
