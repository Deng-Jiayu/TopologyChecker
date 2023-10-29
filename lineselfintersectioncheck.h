#ifndef LINESELFINTERSECTIONCHECK_H
#define LINESELFINTERSECTIONCHECK_H

#include "check.h"
#include "checkerror.h"

class LineSelfIntersectionCheckError : public SingleCheckError
{
public:
    LineSelfIntersectionCheckError(const Check *check,
                                   const QgsGeometry &geometry,
                                   const QgsGeometry &errorLocation,
                                   QgsVertexId vertexId,
                                   const CheckerUtils::SelfIntersection &intersection)
        : SingleCheckError(check, geometry, errorLocation, vertexId), mIntersection(intersection)
    {
    }

    const CheckerUtils::SelfIntersection &intersection() const { return mIntersection; }
    bool isEqual(const SingleCheckError *other) const override;
    bool handleChanges(const QList<Check::Change> &changes) override;
    void update(const SingleCheckError *other) override;

private:
    CheckerUtils::SelfIntersection mIntersection;
};

class LineSelfIntersectionCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(LineSelfIntersectionCheck)
public:
    LineSelfIntersectionCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
        , excludeEndpoint(configuration.value(QStringLiteral("excludeEndpoint")).toBool())
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer *>>();
    }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::LineGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    static QString factoryDescription() { return QStringLiteral("线对象自身相交"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("LineSelfIntersectionCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() SIP_SKIP;

    CheckErrorSingle *convertToCheckError(SingleCheckError *singleCheckError, const CheckerUtils::LayerFeature &layerFeature) const;

    enum ResolutionMethod
    {
        NoChange
    };

    QSet<QgsVectorLayer *> layers;
    bool excludeEndpoint;
};

#endif // LINESELFINTERSECTIONCHECK_H
