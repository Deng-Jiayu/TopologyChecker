#ifndef LINECOVEREDBYBOUNDARYCHECK_H
#define LINECOVEREDBYBOUNDARYCHECK_H

#include "check.h"

class LineCoveredByBoundaryCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(LineCoveredByBoundaryCheck)
public:
    enum ResolutionMethod
    {
        Delete,
        NoChange
    };

    LineCoveredByBoundaryCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        lineLayers = var.value<QSet<QgsVectorLayer *>>();
        var = configuration.value("layersB");
        polygonLayers = var.value<QSet<QgsVectorLayer *>>();
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

    static QString factoryDescription() { return QStringLiteral("线对象没有被面边界覆盖"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("LineCoveredByBoundaryCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() { return Check::FeatureNodeCheck; }

    QSet<QgsVectorLayer *> lineLayers;
    QSet<QgsVectorLayer *> polygonLayers;
};

#endif // LINECOVEREDBYBOUNDARYCHECK_H
