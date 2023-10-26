#ifndef ATTRVALIDCHECK_H
#define ATTRVALIDCHECK_H

#include "check.h"

class AttrValidCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(AttrValidCheck)
public:
    enum ResolutionMethod
    {
        NoChange
    };

    AttrValidCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QVector<QgsVectorLayer *>>();
        attr = configurationValue<QString>("attr");
    }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::PointGeometry, QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription() { return QStringLiteral("属性无效"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("AttrValidCheck"); }
    static Check::CheckType factoryCheckType() { return Check::FeatureCheck; }

    QVector<QgsVectorLayer *> layers;
    QString attr;
};

#endif // ATTRVALIDCHECK_H
