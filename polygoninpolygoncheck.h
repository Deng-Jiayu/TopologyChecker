#ifndef POLYGONINPOLYGONCHECK_H
#define POLYGONINPOLYGONCHECK_H

#include "check.h"
#include "qgsvectorlayer.h"
#include "checkerror.h"

class PolygonInPolygonCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(PolygonInPolygonCheck)
public:
    enum ResolutionMethod
    {
        Delete,
        NoChange
    };

    explicit PolygonInPolygonCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layersA = var.value<QVector<QgsVectorLayer *>>();
        var = configuration.value("layersB");
        layersB = var.value<QVector<QgsVectorLayer *>>();
    }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString id() const override { return factoryId(); }
    QString description() const override { return factoryDescription(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription() { return QStringLiteral("面对象未被包含"); }
    static QString factoryId() { return QStringLiteral("PolygonInPolygonCheck"); }
    static Check::CheckType factoryCheckType() { return Check::FeatureCheck; }

    QVector<QgsVectorLayer *> layersA;
    QVector<QgsVectorLayer *> layersB;
};

#endif // POLYGONINPOLYGONCHECK_H
