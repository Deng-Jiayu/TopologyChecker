#ifndef LINECOVEREDBYLINECHECK_H
#define LINECOVEREDBYLINECHECK_H

#include "check.h"

class LineCoveredByLineCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(LineCoveredByLineCheck)
public:
    enum ResolutionMethod
    {
        Delete,
        NoChange
    };

    LineCoveredByLineCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layersA = var.value<QVector<QgsVectorLayer *>>();
        var = configuration.value("layersB");
        layersB = var.value<QVector<QgsVectorLayer *>>();
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

    static QString factoryDescription() { return QStringLiteral("线对象没有被一条或多条线覆盖"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("LineCoveredByLineCheck"); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() { return Check::FeatureNodeCheck; }

    QVector<QgsVectorLayer *> layersA;
    QVector<QgsVectorLayer *> layersB;
};

#endif // LINECOVEREDBYLINECHECK_H
