#ifndef LINESELFOVERLAPCHECK_H
#define LINESELFOVERLAPCHECK_H

#include "check.h"
#include "checkerror.h"

class LineSelfOverlapError : public CheckError
{
public:
    /**
     * Creates a new overlap check error for \a check and the \a layerFeature combination.
     * The \a geometry and \a errorLocation ned to be in map coordinates.
     * The \a value is the area of the overlapping area in map units.
     * The \a overlappedFeature provides more details about the overlap.
     */
    LineSelfOverlapError(const Check *check,
                         const CheckerUtils::LayerFeature &layerFeature,
                         const QgsGeometry &geometry,
                         const QgsPointXY &errorLocation,
                         QgsVertexId vidx);

    bool isEqual(CheckError *other) const override;

    bool closeMatch(CheckError *other) const override;

    bool handleChanges(const Check::Changes &changes) override;
};

class LineSelfOverlapCheck : public Check
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(GapCheck)
public:
    enum ResolutionMethod
    {
        Delete,
        NoChange
    };

    LineSelfOverlapCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration), asTotal(configuration.value(QStringLiteral("excludeEndpoint")).toBool())
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer *>>();
    }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    QStringList resolutionMethods() const override;
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() { return {QgsWkbTypes::LineGeometry}; }
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP { return factoryCompatibleGeometryTypes().contains(layer->geometryType()); }
    static QString factoryDescription() { return QStringLiteral("一个线对象本身有重叠"); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral("QgsGeometryLineSelfOverlapCheck"); }
    static Check::CheckType factoryCheckType() { return Check::FeatureCheck; }

    QSet<QgsVectorLayer *> layers;
    bool asTotal;
};

#endif // LINESELFOVERLAPCHECK_H
