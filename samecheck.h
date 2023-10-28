#ifndef SAMECHECK_H
#define SAMECHECK_H

#include "checkcontext.h"
#include "check.h"
#include "checkerror.h"

class SameCheckError : public CheckError
{
public:
    //! Constructor
    SameCheckError(const Check *check,
                   const CheckerUtils::LayerFeature &layerFeature,
                   const QgsPointXY &errorLocation,
                   const QMap<QString, FeaturePool *> &featurePools,
                   const QMap<QString, QList<QgsFeatureId>> &duplicates)
        : CheckError(check, layerFeature, errorLocation, QgsVertexId(), duplicatesString(featurePools, duplicates)), mDuplicates(duplicates)
    {
    }

    //! Returns the duplicates
    QMap<QString, QList<QgsFeatureId>> duplicates() const { return mDuplicates; }

    //! Returns if the \a other error is equivalent
    bool isEqual(CheckError *other) const override
    {
        return other->check() == check() &&
               other->layerId() == layerId() &&
               other->featureId() == featureId() &&
               // static_cast: since other->checker() == checker is only true if the types are actually the same
               static_cast<SameCheckError *>(other)->duplicates() == duplicates();
    }

private:
    QMap<QString, QList<QgsFeatureId>> mDuplicates;

    static QString duplicatesString(const QMap<QString, FeaturePool *> &featurePools, const QMap<QString, QList<QgsFeatureId>> &duplicates);
};

class SameCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(SameCheck)
public:
    explicit SameCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer *>>();
        withinLayer = configuration.value(QStringLiteral("excludeEndpoint")).toBool();
        sameNode = configuration.value(QStringLiteral("sameNode")).toBool();
        qDebug() << "SameCheck";
    }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;

    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString description() const override { return factoryDescription(); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PointGeometry, QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription() { return QStringLiteral("重叠对象"); }
    static QString factoryId();
    static Check::CheckType factoryCheckType();

    enum ResolutionMethod
    {
        NoChange,
        RemoveDuplicates
    };

    QSet<QgsVectorLayer *> layers;
    bool withinLayer;
    bool sameNode;

};

#endif // SAMECHECK_H
