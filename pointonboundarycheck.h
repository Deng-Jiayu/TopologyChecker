﻿#ifndef POINTONBOUNDARYCHECK_H
#define POINTONBOUNDARYCHECK_H

#include "check.h"

class PointOnBoundaryCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(PointOnBoundaryCheck)
public:
    PointOnBoundaryCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        pointLayers = var.value<QSet<QgsVectorLayer*>>();
        var = configuration.value("layersB");
        polygonLayers = var.value<QSet<QgsVectorLayer*>>();
    }
    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PointGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    static QString factoryDescription() { return QStringLiteral("点不在面边界上"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("PointOnBoundaryCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() SIP_SKIP;

    enum ResolutionMethod
    {
        NoChange
    };

    QSet<QgsVectorLayer *> pointLayers;
    QSet<QgsVectorLayer *> polygonLayers;
};

#endif // POINTONBOUNDARYCHECK_H
