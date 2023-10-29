#ifndef DANGLECHECK_H
#define DANGLECHECK_H

#include "check.h"

class DangleCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(DangleCheck)
public:
    DangleCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer *>>();
    }
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;

    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString description() const override { return factoryDescription(); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::LineGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription() { return QStringLiteral("线线悬挂"); }
    static QString factoryId() { return QStringLiteral("DangleCheck"); }
    static Check::CheckType factoryCheckType() SIP_SKIP;

    enum ResolutionMethod
    {
        NoChange
    };

    QSet<QgsVectorLayer *> layers;
};

#endif // DANGLECHECK_H
