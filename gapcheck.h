#ifndef GAPCHECK_H
#define GAPCHECK_H

#include "check.h"
#include "checkerror.h"

class GapCheckError : public CheckError
{
public:
    /**
     * Create a new gap check error produced by \a check on the layer \a layerId.
     * The \a geometry of the gap needs to be in map coordinates.
     * The \a neighbors are a map of layer ids and feature ids.
     * The \a area of the gap in map units and the bounding box of the gap in map units too.
     */
    GapCheckError(const Check *check,
                  const QString &layerId,
                  const QgsGeometry &geometry,
                  const QMap<QString, QgsFeatureIds> &neighbors,
                  double area,
                  const QgsRectangle &gapAreaBBox,
                  const QgsRectangle &contextArea)
        : CheckError(check, layerId, FID_NULL, geometry, geometry.constGet()->centroid(), QgsVertexId(), area, ValueArea), mNeighbors(neighbors), mGapAreaBBox(gapAreaBBox), mContextBoundingBox(contextArea)
    {
    }

    QgsRectangle contextBoundingBox() const override;

    /**
     * A map of layers and feature ids of the neighbors of the gap.
     */
    const QMap<QString, QgsFeatureIds> &neighbors() const { return mNeighbors; }

    bool isEqual(CheckError *other) const override;

    bool closeMatch(CheckError *other) const override;

    void update(const CheckError *other) override;

    bool handleChanges(const Check::Changes & /*changes*/) override;

    QgsRectangle affectedAreaBBox() const override;

    QMap<QString, QgsFeatureIds> involvedFeatures() const override;

    QIcon icon() const override;

private:
    QMap<QString, QgsFeatureIds> mNeighbors;
    QgsRectangle mGapAreaBBox;
    QgsRectangle mContextBoundingBox;
};

class GapCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(GapCheck)
public:
    //! Resolution methods for geometry gap checks
    enum ResolutionMethod
    {
        MergeLongestEdge, //!< Merge the gap with the polygon with the longest shared edge.
        NoChange,         //!< Do not handle the error.
        AddToAllowedGaps, //!< Add gap geometry to allowed gaps layer
        CreateNewFeature, //!< Create a new feature with the gap geometry
        MergeLargestArea, //!< Merge with neighbouring polygon with largest area
    };
    Q_ENUM(ResolutionMethod)

    /**
     * The \a configuration accepts a "gapThreshold" key which specifies
     * the maximum gap size in squared map units. Any gaps which are larger
     * than this area are accepted. If "gapThreshold" is set to 0, the check
     * is disabled.
     */
    explicit GapCheck(const CheckContext *context, const QVariantMap &configuration);

    void prepare(const CheckContext *context, const QVariantMap &configuration) override;

    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;

    QList<CheckResolutionMethod> availableResolutionMethods() const override;

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

private:
    enum Condition
    {
        LongestSharedEdge,
        LargestArea
    };

    bool mergeWithNeighbor(const QMap<QString, FeaturePool *> &featurePools,
                           GapCheckError *err, Changes &changes, QString &errMsg, Condition condition) const;

    // const double mGapThresholdMapUnits;
    QgsWeakMapLayerPointer mAllowedGapsLayer;
    std::unique_ptr<QgsVectorLayerFeatureSource> mAllowedGapsSource;
    double mAllowedGapsBuffer = 0;

    QVector<QgsVectorLayer *> layers;
    const double mAreaMin;
    const double mAreaMax;
};

#endif // GAPCHECK_H
