#ifndef POLYGONCOVEREDBYPOLYGONCHECK_H
#define POLYGONCOVEREDBYPOLYGONCHECK_H

#include "check.h"

class PolygonCoveredByPolygonCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(PolygonCoveredByPolygonCheck)
public:
    explicit PolygonCoveredByPolygonCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layersA = var.value<QVector<QgsVectorLayer *>>();
        var = configuration.value("layersB");
        layersB = var.value<QVector<QgsVectorLayer *>>();
    }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;

    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString description() const override { return factoryDescription(); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription() { return QStringLiteral("面未被多个面覆盖"); }
    static QString factoryId();
    static Check::CheckType factoryCheckType();

    enum ResolutionMethod
    {
        NoChange
    };

    QVector<QgsVectorLayer *> layersA;
    QVector<QgsVectorLayer *> layersB;
};

#endif // POLYGONCOVEREDBYPOLYGONCHECK_H
