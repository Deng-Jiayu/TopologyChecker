#ifndef LINELAYERINTERSECTIONCHECK_H
#define LINELAYERINTERSECTIONCHECK_H

#include "check.h"

class LineLayerIntersectionCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(LineLayerIntersectionCheck)
public:
    LineLayerIntersectionCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
        , excludeEndpoint( configuration.value( QStringLiteral( "excludeEndpoint" ) ).toBool() )
    {
        QVariant var = configuration.value("layersA");
        layersA = var.value<QVector<QgsVectorLayer *>>();
        var = configuration.value("layersB");
        layersB = var.value<QVector<QgsVectorLayer *>>();
    }
    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::LineGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    static QString factoryDescription() { return QStringLiteral("不同数据集线对象相交"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("LineLayerIntersectionCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() SIP_SKIP;

    enum ResolutionMethod
    {
        NoChange
    };

    QVector<QgsVectorLayer *> layersA;
    QVector<QgsVectorLayer *> layersB;
    bool excludeEndpoint;
};

#endif // LINELAYERINTERSECTIONCHECK_H
