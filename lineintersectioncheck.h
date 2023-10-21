#ifndef LINEINTERSECTIONCHECK_H
#define LINEINTERSECTIONCHECK_H

#include "check.h"

class LineIntersectionCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(LineIntersectionCheck)
public:
    LineIntersectionCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration), excludeEndpoint(configuration.value(QStringLiteral("excludeEndpoint")).toBool())
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QVector<QgsVectorLayer *>>();
    }
    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::LineGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    static QString factoryDescription() { return QStringLiteral("同一线数据集内不同线对象相交"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("LineIntersectionCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() SIP_SKIP;

    enum ResolutionMethod
    {
        NoChange
    };

    QVector<QgsVectorLayer *> layers;
    bool excludeEndpoint;
};

#endif // LINEINTERSECTIONCHECK_H
