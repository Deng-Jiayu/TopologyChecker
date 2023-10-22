#include "convexhullcheck.h"
#include "checkerror.h"
#include "checkerutils.h"
#include "qgsgeometryengine.h"
#include "qgsgeometryutils.h"

void ConvexHullCheck::collectErrors(const QMap<QString, FeaturePool *> &featurePools, QList<CheckError *> &errors, QStringList &messages, QgsFeedback *feedback, const LayerFeatureIds &ids) const
{
    Q_UNUSED(messages)
    QMap<QString, QgsFeatureIds> featureIds = ids.isEmpty() ? allLayerFeatureIds(featurePools) : ids.toMap();
    CheckerUtils::LayerFeatures layerFeatures(featurePools, featureIds, compatibleGeometryTypes(), feedback, mContext);
    for (const CheckerUtils::LayerFeature &layerFeature : layerFeatures)
    {
        if (std::find(layers.begin(), layers.end(), layerFeature.layer()) == layers.end())
            continue;
        const QgsAbstractGeometry *geom = layerFeature.geometry().constGet();
        std::unique_ptr<QgsGeometryEngine> geomEngine = CheckerUtils::createGeomEngine(geom, mContext->tolerance);
        if (!geomEngine->isValid())
        {
            messages.append(tr("ConvexHull check failed for (%1): the geometry is invalid").arg(layerFeature.id()));
            continue;
        }
        for (int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart)
        {
            const QgsAbstractGeometry *part = CheckerUtils::getGeomPart(geom, iPart);
            if (!checkConvexHull(part))
            {
                errors.append(new CheckError(this, layerFeature, part->centroid(), QgsVertexId(iPart)));
            }
        }
    }
}

// A C++ program to find convex hull of a set of points. Refer
// https://www.geeksforgeeks.org/orientation-3-ordered-points/
// for explanation of orientation()
#include <stack>
#include <stdlib.h>
using namespace std;

struct Point
{
    double x, y;
};

// A global point needed for sorting points with reference
// to the first point Used in compare function of qsort()
Point p0;

// A utility function to find next to top in a stack
Point nextToTop(stack<Point> &S)
{
    Point p = S.top();
    S.pop();
    Point res = S.top();
    S.push(p);
    return res;
}

// A utility function to swap two points
void swap(Point &p1, Point &p2)
{
    Point temp = p1;
    p1 = p2;
    p2 = temp;
}

// A utility function to return square of distance
// between p1 and p2
double distSq(Point p1, Point p2)
{
    return (p1.x - p2.x) * (p1.x - p2.x) +
           (p1.y - p2.y) * (p1.y - p2.y);
}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are collinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int orientation(Point p, Point q, Point r)
{
    double val = (q.y - p.y) * (r.x - q.x) -
                 (q.x - p.x) * (r.y - q.y);

    if (val == 0)
        return 0;             // collinear
    return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// A function used by library function qsort() to sort an array of
// points with respect to the first point
int compare(const void *vp1, const void *vp2)
{
    Point *p1 = (Point *)vp1;
    Point *p2 = (Point *)vp2;

    // Find orientation
    int o = orientation(p0, *p1, *p2);
    if (o == 0)
        return (distSq(p0, *p2) >= distSq(p0, *p1)) ? -1 : 1;

    return (o == 2) ? -1 : 1;
}

// Prints convex hull of a set of n points.
bool convexHull(Point points[], int n)
{
    if (n <= 3)
        return true;

    // Find the bottommost point
    double ymin = points[0].y;
    int min = 0;
    for (int i = 1; i < n; i++)
    {
        double y = points[i].y;

        // Pick the bottom-most or choose the left
        // most point in case of tie
        if ((y < ymin) || (ymin == y &&
                           points[i].x < points[min].x))
            ymin = points[i].y, min = i;
    }

    // Place the bottom-most point at first position
    swap(points[0], points[min]);

    // Sort n-1 points with respect to the first point.
    // A point p1 comes before p2 in sorted output if p2
    // has larger polar angle (in counterclockwise
    // direction) than p1
    p0 = points[0];
    qsort(&points[1], n - 1, sizeof(Point), compare);

    // If two or more points make same angle with p0,
    // Remove all but the one that is farthest from p0
    // Remember that, in above sorting, our criteria was
    // to keep the farthest point at the end when more than
    // one points have same angle.
    int m = 1; // Initialize size of modified array
    for (int i = 1; i < n; i++)
    {
        // Keep removing i while angle of i and i+1 is same
        // with respect to p0
        while (i < n - 1 && orientation(p0, points[i], points[i + 1]) == 0)
            i++;

        points[m] = points[i];
        m++; // Update size of modified array
    }

    // If modified array of points has less than 3 points,
    // convex hull is not possible
    // if (m < 3) return true;

    // Create an empty stack and push first three points
    // to it.
    stack<Point> S;
    S.push(points[0]);
    S.push(points[1]);
    S.push(points[2]);

    // Process remaining n-3 points
    for (int i = 3; i < m; i++)
    {
        // Keep removing top while the angle formed by
        // points next-to-top, top, and points[i] makes
        // a non-left turn
        while (S.size() > 1 && orientation(nextToTop(S), S.top(), points[i]) != 2)
            S.pop();
        S.push(points[i]);
    }

    return S.size() == n;
}

void ConvexHullCheck::fixError(const QMap<QString, FeaturePool *> &featurePools, CheckError *error, int method, const QMap<QString, int> &mergeAttributeIndices, Changes &changes) const
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

    // Check if polygon still exists
    if (!vidx.isValid(geom))
    {
        error->setObsolete();
        return;
    }

    // Check if error still applies
    const QgsAbstractGeometry *part = CheckerUtils::getGeomPart(geom, vidx.part);
    if (checkConvexHull(part))
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
        deleteFeatureGeometryPart(featurePools, error->layerId(), feature, vidx.part, changes);
        error->setFixed(method);
    }
    else if (method == TransformToConvexHull)
    {
        QString errMsg;
        if (transformToConvexHull(featurePools, error->layerId(), feature, vidx.part, method, mergeAttributeIndices[error->layerId()], changes, errMsg))
        {
            error->setFixed(method);
        }
        else
        {
            error->setFixFailed(QString("Failed to merge with neighbor: %1").arg(errMsg));
        }
    }
    else
    {
        error->setFixFailed(("Unknown method"));
    }
}

bool ConvexHullCheck::transformToConvexHull(const QMap<QString, FeaturePool *> &featurePools, const QString &layerId, QgsFeature &feature, int partIdx, int method, int mergeAttributeIndex, Changes &changes, QString &errMsg) const
{
    QgsGeometry featureGeometry = feature.geometry();
    const QgsAbstractGeometry *geom = featureGeometry.constGet();
    std::unique_ptr<QgsGeometryEngine> geomEngine(CheckerUtils::createGeomEngine(CheckerUtils::getGeomPart(geom, partIdx), mContext->reducedTolerance));

    QgsAbstractGeometry *newGeom = geomEngine->convexHull();
    replaceFeatureGeometryPart(featurePools, layerId, feature, partIdx, newGeom, changes);
    return true;
}

QStringList ConvexHullCheck::resolutionMethods() const
{
    static QStringList methods = QStringList()
                                 << QStringLiteral("转化为凸包")
                                 << QStringLiteral("删除要素")
                                 << QStringLiteral("无");
    return methods;
}

bool ConvexHullCheck::checkConvexHull(const QgsAbstractGeometry *geom) const
{
    int n = geom->vertexCount();
    Point *points = new Point[n - 1];
    int i = 0;
    for (auto it = geom->vertices_begin(); it != geom->vertices_end(); ++it)
    {
        points[i].x = (*it).x();
        points[i].y = (*it).y();
        ++i;
        if (i == n - 1)
            break;
    }

    bool ans = convexHull(points, n - 1);
    delete[] points;
    return ans;
}

bool ConvexHullCheck::selfIntersect(const QgsAbstractGeometry *geom, int iPart) const
{
    for (int iRing = 0, nRings = geom->ringCount(iPart); iRing < nRings; ++iRing)
    {
        if (QgsGeometryUtils::selfIntersections(geom, iPart, iRing, mContext->tolerance).size() != 0)
            return true;
    }
    return false;
}
