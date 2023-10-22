﻿#ifndef LINEINPOLYGONCHECK_H
#define LINEINPOLYGONCHECK_H

#include "check.h"

class LineInPolygonCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(LineInPolygonCheck)
public:
    enum ResolutionMethod
    {
        Delete,
        NoChange
    };

    LineInPolygonCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        lineLayers = var.value<QVector<QgsVectorLayer *>>();
        var = configuration.value("layersB");
        polygonLayers = var.value<QVector<QgsVectorLayer *>>();
    }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::LineGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;

    QStringList resolutionMethods() const override
    {
        static QStringList methods = QStringList()
                                     << QStringLiteral("删除要素")
                                     << QStringLiteral("无");
        return methods;
    }

    static QString factoryDescription() { return QStringLiteral("线对象与面存在交集"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("LineInPolygonCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() { return Check::FeatureNodeCheck; }

    QVector<QgsVectorLayer *> lineLayers;
    QVector<QgsVectorLayer *> polygonLayers;
};

#endif // LINEINPOLYGONCHECK_H
