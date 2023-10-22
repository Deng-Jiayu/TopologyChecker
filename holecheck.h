#ifndef HOLECHECK_H
#define HOLECHECK_H

#include "check.h"

class HoleCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(HoleCheck)
public:
    explicit HoleCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QVector<QgsVectorLayer *>>();
    }
    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    static QString factoryDescription() { return QStringLiteral("面内有洞"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("HoleCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() { return Check::FeatureCheck; }

    enum ResolutionMethod
    {
        RemoveHoles,
        NoChange
    };

    QVector<QgsVectorLayer *> layers;
};

#endif // HOLECHECK_H
