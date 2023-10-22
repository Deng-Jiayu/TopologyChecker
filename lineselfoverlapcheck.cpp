#include "lineselfoverlapcheck.h"
#include "checkerutils.h"
#include <qgsgeometryengine.h>
#include "checkerror.h"

LineSelfOverlapError::LineSelfOverlapError(const Check *check, const CheckerUtils::LayerFeature &layerFeature, const QgsGeometry &geometry, const QgsPointXY &errorLocation, QgsVertexId vidx)
    : CheckError(check, layerFeature.layer()->id(), layerFeature.feature().id(), geometry, errorLocation, vidx)
{
}

bool LineSelfOverlapError::isEqual(CheckError *other) const
{
    LineSelfOverlapError *err = dynamic_cast<LineSelfOverlapError *>(other);
    return err &&
           other->layerId() == layerId() &&
           other->featureId() == featureId() &&
           CheckerUtils::pointsFuzzyEqual(location(), other->location(), mCheck->context()->reducedTolerance);
}

bool LineSelfOverlapError::closeMatch(CheckError *other) const
{
    LineSelfOverlapError *err = dynamic_cast<LineSelfOverlapError *>(other);
    return err && other->layerId() == layerId() && other->featureId() == featureId();
}

bool LineSelfOverlapError::handleChanges(const Check::Changes &changes)
{
    if (!CheckError::handleChanges(changes))
    {
        return false;
    }
    return true;
}

void LineSelfOverlapCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();

    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);

    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        qDebug() << layerFeature.id();
        QVector<LineSegment> totalLines;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            const QgsAbstractGeometry *lineA = CheckerUtils::getGeomPart(geom, iPart);
            if (lineA->vertexCount() <= 2)
                continue;

            QVector<LineSegment> lines;
            auto i = lineA->vertices_begin();
            QgsPoint first = *i;
            for (++i; i != lineA->vertices_end(); ++i)
            {
                LineSegment line(first, *i);
                lines.push_back(line);
                first = *i;
            }
            if (asTotal)
            {
                totalLines.append(lines);
                continue;
            }

            std::sort(lines.begin(), lines.end(), CheckerUtils::cmp);

            QVector<QgsGeometry> ans = CheckerUtils::lineSelfOverlay(lines, mContext->tolerance);
            if (ans.empty())
                continue;
            QgsGeometry conflict;
            for (auto k = ans.begin(); k != ans.end(); ++k)
            {
                if (conflict.isEmpty())
                    conflict = *k;
                else
                    conflict = conflict.combine(*k);
            }
            errors.append(new LineSelfOverlapError(this, layerFeature, conflict, conflict.centroid().asPoint(), QgsVertexId(iPart)));
        }
        if (!asTotal)
            continue;
        std::sort(totalLines.begin(), totalLines.end(), CheckerUtils::cmp);

        QVector<QgsGeometry> ans = CheckerUtils::lineSelfOverlay(totalLines, mContext->tolerance);
        if (ans.empty())
            continue;
        QgsGeometry conflict;
        for (auto k = ans.begin(); k != ans.end(); ++k)
        {
            if (conflict.isEmpty())
                conflict = *k;
            else
                conflict = conflict.combine(*k);
        }
        errors.append(new LineSelfOverlapError(this, layerFeature, conflict, conflict.centroid().asPoint(), QgsVertexId()));
    }
}

void LineSelfOverlapCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }

    const QgsGeometry g = feature.geometry();
    const QgsAbstractGeometry *geom = g.constGet();
    QgsVertexId vidx = error->vidx();

    // Check if line still exists
    if (!vidx.isValid(geom) && !asTotal)
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsAbstractGeometry *lineA = CheckerUtils::getGeomPart(geom, vidx.part);
    if(asTotal) lineA = geom;
    if (lineA->vertexCount() <= 1)
    {
        error->setObsolete();
        return;
    }
    QVector<LineSegment> lines;
    auto i = lineA->vertices_begin();
    QgsPoint first = *i;
    for (++i; i != lineA->vertices_end(); ++i)
    {
        LineSegment line(first, *i);
        lines.push_back(line);
        first = *i;
    }
    std::sort(lines.begin(), lines.end(), CheckerUtils::cmp);
    QVector<QgsGeometry> ans = CheckerUtils::lineSelfOverlay(lines, mContext->tolerance);
    if (ans.empty())
    {
        error->setObsolete();
        return;
    }

    // Fix with selected method
    if (method == NoChange)
    {
        error->setFixed(method);
    }
    else if (method == Delete)
    {
        if(asTotal) {
            changes[error->layerId()][feature.id()].append( Change( ChangeFeature, ChangeRemoved ) );
            featurePool->deleteFeature( feature.id() );
            error->setFixed( method );
        } else {
            deleteFeatureGeometryPart(featurePools, error->layerId(), feature, vidx.part, changes);
            error->setFixed(method);
        }
    }
    else
    {
        error->setFixFailed(tr("Unknown method"));
    }
}

QStringList LineSelfOverlapCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("删除要素")
                                 << QStringLiteral("无");
    return methods;
}
