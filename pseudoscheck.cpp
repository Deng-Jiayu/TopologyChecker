#include "pseudoscheck.h"
#include "checkcontext.h"
#include "qgsgeometryutils.h"
#include "featurepool.h"
#include "checkerror.h"

class PointComparer
{
public:
    bool operator()(const QgsPointXY &p1, const QgsPointXY &p2) const
    {
        if (p1.x() < p2.x())
        {
            return true;
        }

        if (p1.x() == p2.x() && p1.y() < p2.y())
        {
            return true;
        }

        return false;
    }
};

void PseudosCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)

    for (auto it : featurePools)
    {

        QgsVectorLayer *layer = it->layer();

        QgsPointXY startPoint;
        QgsPointXY endPoint;

        std::multimap<QgsPointXY, QgsFeatureId, PointComparer> endVerticesMap;

        QgsFeature feature;
        QgsFeatureRequest req;
        QgsFeatureIds featureIds = it->allFeatureIds();
        req.setFilterFids(featureIds);
        QgsFeatureIterator iterator = it->layer()->getFeatures(req);
        while (iterator.nextFeature(feature))
        {
            if (!feature.hasGeometry() || feature.geometry().isNull())
                continue;

            QgsGeometry geo = feature.geometry();

            if (geo.isMultipart())
            {
                QgsMultiPolylineXY lines = geo.asMultiPolyline();
                for (int m = 0; m < lines.count(); m++)
                {
                    QgsPolylineXY line = lines[m];
                    startPoint = line[0];
                    endPoint = line[line.size() - 1];

                    endVerticesMap.insert(std::pair<QgsPointXY, QgsFeatureId>(startPoint, feature.id()));
                    endVerticesMap.insert(std::pair<QgsPointXY, QgsFeatureId>(endPoint, feature.id()));
                }
            }
            else
            {
                QgsPolylineXY polyline = geo.asPolyline();
                startPoint = polyline[0];
                endPoint = polyline[polyline.size() - 1];
                endVerticesMap.insert(std::pair<QgsPointXY, QgsFeatureId>(startPoint, feature.id()));
                endVerticesMap.insert(std::pair<QgsPointXY, QgsFeatureId>(endPoint, feature.id()));
            }
        }

        for (std::multimap<QgsPointXY, QgsFeatureId, PointComparer>::iterator pointIt = endVerticesMap.begin(), end = endVerticesMap.end(); pointIt != end; pointIt = endVerticesMap.upper_bound(pointIt->first))
        {
            QgsPointXY p = pointIt->first;
            QgsFeatureId k = pointIt->second;

            size_t repetitions = endVerticesMap.count(p);

            if (repetitions == 2)
            {
                QgsGeometry conflictGeom = QgsGeometry::fromPointXY(p);

                QgsFeature feat;
                layer->getFeatures(QgsFeatureRequest().setFilterFid(k)).nextFeature(feat);

                errors.append(new CheckError(this, layer->id(), feature.id(), conflictGeom, p));
            }
        }
    }
}

void PseudosCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> & /*mergeAttributeIndices*/, Changes &changes) const
{
    FeaturePool *featurePool = featurePools[error->layerId()];
    QgsFeature feature;
    if (!featurePool->getFeature(error->featureId(), feature))
    {
        error->setObsolete();
        return;
    }
    QgsGeometry featureGeom = feature.geometry();
    QgsAbstractGeometry *geom = featureGeom.get();
    QgsVertexId vidx = error->vidx();

    // Check if point still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies

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

QStringList PseudosCheck::resolutionMethods() const
{
    static QStringList methods = QStringList() << QStringLiteral("无");
    return methods;
}

Check::CheckType PseudosCheck::factoryCheckType()
{
    return Check::FeatureNodeCheck;
}
