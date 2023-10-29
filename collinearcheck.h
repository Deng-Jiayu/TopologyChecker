﻿#ifndef COLLINEARCHECK_H
#define COLLINEARCHECK_H

#include "check.h"

class CollinearCheck : public Check
{
    Q_DECLARE_TR_FUNCTIONS( CollinearCheck )
public:
    enum ResolutionMethod
    {
        DeleteNode,
        NoChange
    };

    CollinearCheck( CheckContext *context, const QVariantMap &configuration )
        : Check( context, configuration )
    {
        QVariant var = configuration.value("layersA");
        layers = var.value<QSet<QgsVectorLayer*>>();
    }

    void collectErrors( const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids = LayerFeatureIds() ) const override;
    void fixError( const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes ) const override;

    QList<QgsWkbTypes::GeometryType> compatibleGeometryTypes() const override;
    Q_DECL_DEPRECATED QStringList resolutionMethods() const override;
    QString id() const override;
    QString description() const override;
    Check::CheckType checkType() const override;

    static QList<QgsWkbTypes::GeometryType> factoryCompatibleGeometryTypes() SIP_SKIP;
    static bool factoryIsCompatible( QgsVectorLayer *layer ) SIP_SKIP;
    static QString factoryDescription() SIP_SKIP;
    static QString factoryId() SIP_SKIP;
    static Check::CheckType factoryCheckType() SIP_SKIP;

    QSet<QgsVectorLayer *> layers;
};

#endif // COLLINEARCHECK_H
