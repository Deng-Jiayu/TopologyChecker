#ifndef POINTDUPLICATECHECK_H
#define POINTDUPLICATECHECK_H

#include "checkcontext.h"
#include "check.h"
#include "checkerror.h"

class PointDuplicateCheckError : public CheckError
{
public:
    PointDuplicateCheckError(const Check *check,
                             const CheckerUtils::LayerFeature &layerFeature,
                             const QgsPointXY &errorLocation,
                             const QMap<QString, FeaturePool *> &featurePools,
                             const QMap<QString, QList<QgsFeatureId>> &duplicates)
        : CheckError(check, layerFeature, errorLocation, QgsVertexId(), duplicatesString(featurePools, duplicates)), mDuplicates(duplicates)
    {
    }
    QMap<QString, QList<QgsFeatureId>> duplicates() const { return mDuplicates; }

    bool isEqual(CheckError *other) const override
    {
        return other->check() == check() &&
               other->layerId() == layerId() &&
               other->featureId() == featureId() &&
               // static_cast: since other->checker() == checker is only true if the types are actually the same
               static_cast<PointDuplicateCheckError *>(other)->duplicates() == duplicates();
    }

private:
    QMap<QString, QList<QgsFeatureId>> mDuplicates;

    static QString duplicatesString(const QMap<QString, FeaturePool *> &featurePools, const QMap<QString, QList<QgsFeatureId>> &duplicates);
};

class PointDuplicateCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(PointDuplicateCheck)
public:
    explicit PointDuplicateCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        pointLayers = var.value<QVector<QgsVectorLayer *>>();
    }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;

    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString description() const override { return factoryDescription(); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PointGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription() { return QStringLiteral("重复点"); }
    static QString factoryId();
    static Check::CheckType factoryCheckType();

    enum ResolutionMethod
    {
        NoChange,
        RemoveDuplicates
    };

    QVector<QgsVectorLayer *> pointLayers;
};

#endif // POINTDUPLICATECHECK_H
