#include "lineselfintersectioncheck.h"
#include "checkcontext.h"
#include "qgsvectorlayer.h"
#include "checkerror.h"
#include "checkerutils.h"

bool LineSelfIntersectionCheckError::isEqual(const SingleCheckError *other) const
{
    return SingleCheckError::isEqual(other) &&
           static_cast<const LineSelfIntersectionCheckError *>(other)->intersection().segment1 == intersection().segment1 &&
           static_cast<const LineSelfIntersectionCheckError *>(other)->intersection().segment2 == intersection().segment2;
}

bool LineSelfIntersectionCheckError::handleChanges(const QList<Check::Change> &changes)
{
    if (!SingleCheckError::handleChanges(changes))
        return false;

    for (const Check::Change &change : changes)
    {
        if (change.vidx.vertex == mIntersection.segment1 ||
            change.vidx.vertex == mIntersection.segment1 + 1 ||
            change.vidx.vertex == mIntersection.segment2 ||
            change.vidx.vertex == mIntersection.segment2 + 1)
        {
            return false;
        }
        else if (change.vidx.vertex >= 0)
        {
            if (change.vidx.vertex < mIntersection.segment1)
            {
                mIntersection.segment1 += change.type == Check::ChangeAdded ? 1 : -1;
            }
            if (change.vidx.vertex < mIntersection.segment2)
            {
                mIntersection.segment2 += change.type == Check::ChangeAdded ? 1 : -1;
            }
        }
    }
    return true;
}

void LineSelfIntersectionCheckError::update(const SingleCheckError *other)
{
    SingleCheckError::update(other);
    // Static cast since this should only get called if isEqual == true
    const LineSelfIntersectionCheckError *err = static_cast<const LineSelfIntersectionCheckError *>(other);
    mIntersection.point = err->mIntersection.point;
}

void LineSelfIntersectionCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeaturesA(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext, true);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeaturesA)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        QgsGeometry geometry = layerFeature.geometry();
        const QgsAbstractGeometry *geom = geometry.constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            for (int iRing = 0, nRings = geom->ringCount(iPart); iRing < nRings; ++iRing)
            {
                for (const CheckerUtils::SelfIntersection &inter : (CheckerUtils::selfIntersections(geom, iPart, iRing, mContext->tolerance, !excludeEndpoint)))
                {
                    SingleCheckError *error = new LineSelfIntersectionCheckError(this, geometry, QgsGeometry(inter.point.clone()), QgsVertexId(iPart, iRing), inter);
                    errors.append(convertToGeometryCheckError(error, layerFeature));
                }
            }
        }
    }
}

void LineSelfIntersectionCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & /*changes*/) const
{
    Q_UNUSED(featurePools)

    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList LineSelfIntersectionCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
    return methods;
}

Check::CheckType LineSelfIntersectionCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}

CheckErrorSingle *LineSelfIntersectionCheck::convertToGeometryCheckError(SingleCheckError *singleCheckError, const CheckerUtils::LayerFeature &layerFeature) const
{
    return new CheckErrorSingle(singleCheckError, layerFeature);
}
