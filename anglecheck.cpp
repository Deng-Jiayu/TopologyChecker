#include "checkcontext.h"
#include "anglecheck.h"
#include "qgsgeometryutils.h"
#include "featurepool.h"
#include "checkerror.h"

QList<QgsWkbTypes::GeometryType> AngleCheck::compatibleGeometryTypes() const
{
    return factoryCompatibleGeometryTypes();
}

void AngleCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, context());
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            for (int iRing = 0, nRings = geom->ringCount(iPart); iRing < nRings; ++iRing)
            {
                bool closed = false;
                int nVerts = CheckerUtils::polyLineSize(geom, iPart, iRing, &closed);
                // Less than three points, no angles to check
                if (nVerts < 3)
                {
                    continue;
                }
                for (int iVert = !closed; iVert < nVerts - !closed; ++iVert)
                {
                    const QgsPoint &p1 = geom->vertexAt(QgsVertexId(iPart, iRing, (iVert - 1 + nVerts) % nVerts));
                    const QgsPoint &p2 = geom->vertexAt(QgsVertexId(iPart, iRing, iVert));
                    const QgsPoint &p3 = geom->vertexAt(QgsVertexId(iPart, iRing, (iVert + 1) % nVerts));
                    QgsVector v21, v23;
                    try
                    {
                        v21 = QgsVector(p1.x() - p2.x(), p1.y() - p2.y()).normalized();
                        v23 = QgsVector(p3.x() - p2.x(), p3.y() - p2.y()).normalized();
                    }
                    catch (const QgsException &)
                    {
                        // Zero length vectors
                        continue;
                    }

                    double angle = std::acos(v21 * v23) / M_PI * 180.0;
                    if (angle < mMinAngle)
                    {
                        errors.append(new CheckError(this, layerFeature, p2, QgsVertexId(iPart, iRing, iVert), angle));
                    }
                }
            }
        }
    }
}

void AngleCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }
    QgsGeometry featureGeometry = feature.geometry();
    QgsAbstractGeometry *geometry = featureGeometry.get();
    QgsVertexId vidx = error->vidx();

    // Check if point still exists
    if (!vidx.isValid(geometry))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    int n = CheckerUtils::polyLineSize(geometry, vidx.part, vidx.ring);
    if (n == 0)
    {
        error->setObsolete();
        return;
    }
    const QgsPoint &p1 = geometry->vertexAt(QgsVertexId(vidx.part, vidx.ring, (vidx.vertex - 1 + n) % n));
    const QgsPoint &p2 = geometry->vertexAt(vidx);
    const QgsPoint &p3 = geometry->vertexAt(QgsVertexId(vidx.part, vidx.ring, (vidx.vertex + 1) % n));
    QgsVector v21, v23;
    try
    {
        v21 = QgsVector(p1.x() - p2.x(), p1.y() - p2.y()).normalized();
        v23 = QgsVector(p3.x() - p2.x(), p3.y() - p2.y()).normalized();
    }
    catch (const QgsException &)
    {
        error->setObsolete();
        return;
    }
    double angle = std::acos(v21 * v23) / M_PI * 180.0;
    if (angle >= mMinAngle)
    {
        error->setObsolete();
        return;
    }

    // Fix error
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == DeleteNode)
    {
        if (!CheckerUtils::canDeleteVertex(geometry, vidx.part, vidx.ring))
        {
            error->setFixFailed(QStringLiteral("生成的几何是退化的"));
        }
        else if (!geometry->deleteVertex(error->vidx()))
        {
            error->setFixFailed(QStringLiteral("失败"));
        }
        else
        {
            changes[error->layerId()][error->featureId()].append(Change(ChangeNode, ChangeRemoved, vidx));
            // Avoid duplicate nodes as result of deleting spike vertex
            if (QgsGeometryUtils::sqrDistance2D(p1, p3) < mContext->tolerance &&
                CheckerUtils::canDeleteVertex(geometry, vidx.part, vidx.ring) &&
                geometry->deleteVertex(error->vidx())) // error->vidx points to p3 after removing p2
            {
                changes[error->layerId()][error->featureId()].append(Change(ChangeNode, ChangeRemoved, QgsVertexId(vidx.part, vidx.ring, (vidx.vertex + 1) % n)));
            }
            feature.setGeometry(featureGeometry);
            featurePool->updateFeature(feature);
            error->setFixed(method);
        }
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList AngleCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("删除节点") << QStringLiteral("无");
    return methods;
}

QString AngleCheck::id() const
{
    return factoryId();
}

QString AngleCheck::factoryDescription()
{
    return QStringLiteral("面内锐角");
}

QString AngleCheck::description() const
{
    return factoryDescription();
}

Check::CheckType AngleCheck::checkType() const
{
    return factoryCheckType();
}

QList<QgsWkbTypes::GeometryType> AngleCheck::factoryCompatibleGeometryTypes()
{
    return {QgsWkbTypes::PolygonGeometry};
}

bool AngleCheck::factoryIsCompatible(QgsVectorLayer *layer)
{
    return factoryCompatibleGeometryTypes().contains(layer->geometryType());
}

QString AngleCheck::factoryId()
{
    return QStringLiteral("AngleCheck");
}

Check::CheckType AngleCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
