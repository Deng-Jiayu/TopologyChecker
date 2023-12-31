﻿#ifndef LENGTHCHECK_H
#define LENGTHCHECK_H

#include "check.h"

class LengthCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(LengthCheck)
public:
    LengthCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration), mLengthMin(configuration.value("lengthMin").toDouble()), mLengthMax(configuration.value("lengthMax").toDouble())
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer *>>();
    }
    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    static QString factoryDescription() { return QStringLiteral("线长度"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("QgsGeometryLengthCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() SIP_SKIP;

    enum ResolutionMethod
    {
        NoChange
    };

    QSet<QgsVectorLayer *> layers;
    const double mLengthMin;
    const double mLengthMax;
};

#endif // LENGTHCHECK_H
