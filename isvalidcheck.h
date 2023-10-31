#ifndef ISVALIDCHECK_H
#define ISVALIDCHECK_H

#include "check.h"
#include "checkerror.h"

class IsValidCheckError : public SingleCheckError
{
public:
    /**
     * Creates a new is valid check error.
     */
    IsValidCheckError(const Check *check, const QgsGeometry &geometry, const QgsGeometry &errorLocation, const QString &errorDescription);

    QString description() const override;

private:
    QString mDescription;
};

class IsValidCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS(IsValidCheck)
public:
    enum ResolutionMethod
    {
        MakeValid, NoChange
    };

    IsValidCheck(CheckContext *context, const QVariantMap &configuration)
        : Check(context, configuration)
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer *>>();
        GEOS = configuration.value(QStringLiteral("GEOS")).toBool();
    }

    void collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds()) const override;
    void fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const override;

    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString id() const override;
    QString description() const override;
    Check::CheckType checkType() const override;

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() SIP_SKIP;
    static bool factoryIsCompatible(QgsVectorLayer *layer) SIP_SKIP;
    static QString factoryDescription() SIP_SKIP;
    static QString factoryId() SIP_SKIP;
    static Check::CheckType factoryCheckType() SIP_SKIP;

    CheckErrorSingle *convertToCheckError(SingleCheckError *error, const CheckerUtils::LayerFeature &layerFeature) const;
    QSet<QgsVectorLayer *> layers;
    bool GEOS;
};

#endif // ISVALIDCHECK_H
