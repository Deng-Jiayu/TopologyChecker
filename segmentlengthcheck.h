#ifndef SEGMENTLENGTHCHECK_H
#define SEGMENTLENGTHCHECK_H

#include "check.h"

class SegmentLengthCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(SegmentLengthCheck)
public:
    SegmentLengthCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration), mLengthMin(configuration.value("lengthMin").toDouble()), mLengthMax(configuration.value("lengthMax").toDouble())
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QVector<QgsVectorLayer *>>();
    }
    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    static QString factoryDescription() { return QStringLiteral("线段长度"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("QgsGeometrySegmentLengthCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() SIP_SKIP;

    enum ResolutionMethod
    {
        NoChange
    };

    QVector<QgsVectorLayer *> layers;
    const double mLengthMin;
    const double mLengthMax;
};

#endif // SEGMENTLENGTHCHECK_H
