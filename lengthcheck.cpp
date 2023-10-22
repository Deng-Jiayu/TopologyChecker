#include "lengthcheck.h"
#include "checkcontext.h"
#include "featurepool.h"
#include "checkerror.h"

void LengthCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        double layerToMapUnits = scaleFactor(layerFeature.layer());
        double minLength = mLengthMin / layerToMapUnits;
        double maxLength = mLengthMax / layerToMapUnits;

        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            double dist = 0.00;
            for (int iRing = 0, nRings = geom->ringCount(iPart); iRing < nRings; ++iRing)
            {
                bool isClosed = false;
                int nVerts = CheckerUtils::polyLineSize(geom, iPart, iRing, &isClosed);
                if (nVerts < 2)
                {
                    continue;
                }
                for (int iVert = isClosed ? 0 : 1, jVert = isClosed ? nVerts - 1 : 0; iVert < nVerts; jVert = iVert++)
                {
                    QgsPoint pi = geom->vertexAt(QgsVertexId(iPart, iRing, iVert));
                    QgsPoint pj = geom->vertexAt(QgsVertexId(iPart, iRing, jVert));
                    dist += pi.distance(pj);
                }
            }
            if (dist >= minLength && dist <= maxLength)
            {
                const QgsAbstractGeometry *g = CheckerUtils::getGeomPart(geom, iPart);
                errors.append(new CheckError(this, layerFeature, g->centroid(), QgsVertexId(iPart), dist * layerToMapUnits, CheckError::ValueLength));
            }
        }
    }
}

void LengthCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes & /*changes*/) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }

    QgsGeometry featureGeom = feature.geometry();
    const QgsAbstractGeometry *geom = featureGeom.constGet();
    QgsVertexId vidx = error->vidx();

    // Check if point still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    int iPart = vidx.part;
    double layerToMapUnits = scaleFactor(featurePool->layer());
    double minLength = mLengthMin / layerToMapUnits;
    double maxLength = mLengthMax / layerToMapUnits;
    double dist = 0.00;
    for (int iRing = 0, nRings = geom->ringCount(iPart); iRing < nRings; ++iRing)
    {
        bool isClosed = false;
        int nVerts = CheckerUtils::polyLineSize(geom, iPart, iRing, &isClosed);
        if (nVerts < 2)
        {
            continue;
        }
        for (int iVert = isClosed ? 0 : 1, jVert = isClosed ? nVerts - 1 : 0; iVert < nVerts; jVert = iVert++)
        {
            QgsPoint pi = geom->vertexAt(QgsVertexId(iPart, iRing, iVert));
            QgsPoint pj = geom->vertexAt(QgsVertexId(iPart, iRing, jVert));
            dist += pi.distance(pj);
        }
    }
    if (dist <= minLength || dist >= maxLength)
    {
        error->setObsolete();
        return;
    }

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList LengthCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
    return methods;
}

Check::CheckType LengthCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
