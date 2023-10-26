#ifndef UNIQUEATTRCHECK_H
#define UNIQUEATTRCHECK_H

#include "check.h"
#include "checkerror.h"

class AttrDuplicateError : public CheckError
{
public:
    AttrDuplicateError( const Check *check,
                                   const CheckerUtils::LayerFeature &layerFeature,
                                   const QgsPointXY &errorLocation,
                                   const QMap<QString, FeaturePool *> &featurePools,
                                   const QList<QgsFeatureId> &duplicates )
        : CheckError( check, layerFeature, errorLocation, QgsVertexId(), duplicatesString( featurePools, duplicates ) )
        , mDuplicates( duplicates )
    { }
    QList<QgsFeatureId> duplicates() const { return mDuplicates; }

    bool isEqual( CheckError *other ) const override
    {
        return other->check() == check() &&
               other->layerId() == layerId() &&
               other->featureId() == featureId() &&
               // static_cast: since other->checker() == checker is only true if the types are actually the same
               static_cast<AttrDuplicateError *>( other )->duplicates() == duplicates();
    }

private:
    QList<QgsFeatureId> mDuplicates;

    static QString duplicatesString( const QMap<QString, FeaturePool *> &featurePools, const QList<QgsFeatureId> &duplicates );
};

class UniqueAttrCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS( UniqueAttrCheck )
public:
    explicit UniqueAttrCheck( CheckContext *context, const QVariantMap &configuration )
        : Check( context, configuration ) {}
    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() {return {QgsWkbTypes::PointGeometry, QgsWkbTypes::LineGeometry, QgsWkbTypes::PolygonGeometry}; }
    static bool factoryIsCompatible( QgsVectorLayer *layer ) SIP_SKIP { return factoryCompatibleGeometryTypes().contains( layer->geometryType() ); }
    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override { return factoryCompatibleGeometryTypes(); }
    void collectErrors( const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds() ) const override;
    void fixError( const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes ) const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    static QString factoryDescription() { return QStringLiteral( "检查唯一标识码" ); }
    QString description() const override { return factoryDescription(); }
    static QString factoryId() { return QStringLiteral( "UniqueAttrCheck" ); }
    QString id() const override { return factoryId(); }
    Check::CheckType checkType() const override { return factoryCheckType(); }
    static Check::CheckType factoryCheckType() { return Check::FeatureCheck; }

    enum ResolutionMethod { NoChange };
};

#endif // UNIQUEATTRCHECK_H
