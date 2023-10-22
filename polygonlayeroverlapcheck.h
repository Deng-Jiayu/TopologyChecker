#ifndef POLYGONLAYEROVERLAPCHECK_H
#define POLYGONLAYEROVERLAPCHECK_H

#include "check.h"
#include "checkerror.h"

class PolygonLayerOverlapCheckError : public CheckError
{
public:
    struct OverlappedFeature
    {
    public:
        OverlappedFeature(QgsVectorLayer *vl, QgsFeatureId fid)
            : mLayerId(vl->id()), mLayerName(vl->name()), mFeatureId(fid)
        {
        }

        QString layerId() const { return mLayerId; }
        QString layerName() const { return mLayerName; }
        QgsFeatureId featureId() const { return mFeatureId; }
        bool operator==(const OverlappedFeature &other) const { return mLayerId == other.layerId() && mFeatureId == other.featureId(); }

    private:
        QString mLayerId;
        QString mLayerName;
        QgsFeatureId mFeatureId;
    };

    /**
     * Creates a new overlap check error for \a check and the \a layerFeature combination.
     * The \a geometry and \a errorLocation ned to be in map coordinates.
     * The \a value is the area of the overlapping area in map units.
     * The \a overlappedFeature provides more details about the overlap.
     */
    PolygonLayerOverlapCheckError(const Check *check,
                                  const CheckerUtils::LayerFeature &layerFeature,
                                  const QgsGeometry &geometry,
                                  const QgsPointXY &errorLocation,
                                  const QVariant &value,
                                  const CheckerUtils::LayerFeature &overlappedFeature);

    /**
     * Returns the overlapped feature
     */
    const OverlappedFeature &overlappedFeature() const { return mOverlappedFeature; }

    bool isEqual(CheckError *other) const override;

    bool closeMatch(CheckError *other) const override;

    bool handleChanges(const Check::Changes &changes) override;

    QString description() const override;

    QMap<QString, QgsFeatureIds> involvedFeatures() const override;
    QIcon icon() const override;

private:
    OverlappedFeature mOverlappedFeature;
};

class PolygonLayerOverlapCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(PolygonLayerOverlapCheck)
public:
    /**
     * Available resolution methods.
     */
    enum ResolutionMethod
    {
        Subtract, //!< Subtract the overlap region from the polygon
        NoChange  //!< Do not change anything
    };

    /**
     * Checks for overlapping polygons.
     *
     * In \a configuration a maxOverlapArea parameter can be passed. In case this parameter is set
     * to something else than 0.0, the error will only be reported if the overlapping area is smaller
     * than maxOverlapArea.
     * Overlapping areas smaller than the reducedTolerance parameter of the \a context are ignored.
     */
    PolygonLayerOverlapCheck(const CheckContext *context, const QVariantMap &configuration);
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;

    QString description() const override;
    QString id() const override;
    Check::Flags flags() const override;
    Check::CheckType checkType() const override { return factoryCheckType(); }

    static QString factoryDescription() SIP_SKIP;
    static QString factoryId() SIP_SKIP;
    static Check::Flags factoryFlags() SIP_SKIP;
    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() SIP_SKIP;
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP;
    static Check::CheckType factoryCheckType() SIP_SKIP;

    const double mOverlapThresholdMapUnits;
    QVector<QgsVectorLayer *> layersA;
    QVector<QgsVectorLayer *> layersB;
};

#endif // POLYGONLAYEROVERLAPCHECK_H
