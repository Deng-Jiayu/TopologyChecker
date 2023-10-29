#ifndef AREACHECK_H
#define AREACHECK_H

#include "check.h"

class QgsSurface;

class AreaCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(AreaCheck)
public:
    enum ResolutionMethod
    {
        MergeLongestEdge,
        MergeLargestArea,
        Delete,
        NoChange
    };

    AreaCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration),
          mAreaMin(configurationValue<double>("areaMin")),
          mAreaMax(configurationValue<double>("areaMax"))
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer *>>();
    }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription() { return QStringLiteral("面积检查"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("AreaCheck"); }
    static Check::CheckType factoryCheckType() { return Check::FeatureCheck; }

    QSet<QgsVectorLayer *> layers;

private:
    virtual bool checkThreshold(double layerToMapUnits, const QgsAbstractGeometry *geom, double &value) const;
    bool mergeWithNeighbor(const QMap<QString, FeaturePool *> &featurePools, const QString &layerId, QgsFeature &feature, int partIdx, int method, int mergeAttributeIndex, Changes &changes, QString &errMsg) const;

    const double mAreaMin;
    const double mAreaMax;
};

#endif // AREACHECK_H
