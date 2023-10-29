#ifndef CONVEXHULLCHECK_H
#define CONVEXHULLCHECK_H

#include "check.h"
#include "checkerutils.h"

class ConvexHullCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(ConvexHullCheck)
public:
    enum ResolutionMethod
    {
        TransformToConvexHull,
        Delete,
        NoChange
    };

    ConvexHullCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer *>>();
        reverse = configuration.value(QStringLiteral("excludeEndpoint")).toBool();
    }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    QStringList resolutionMethods() const override;
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription()
    {
        if(reverse) return QStringLiteral("凸包");
        else return QStringLiteral("非凸包");
    }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("ConvexHullCheck"); }
    static Check::CheckType factoryCheckType() { return Check::FeatureCheck; }

    QSet<QgsVectorLayer *> layers;
    static bool reverse;

private:
    bool transformToConvexHull(const QMap<QString, FeaturePool *> &featurePools, const QString &layerId, QgsFeature &feature, int partIdx, int method, int mergeAttributeIndex, Changes &changes, QString &errMsg) const;
};

#endif // CONVEXHULLCHECK_H
