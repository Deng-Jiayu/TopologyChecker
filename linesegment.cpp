#include "linesegment.h"

LineSegment::LineSegment(const QgsPointXY &start, const QgsPointXY &end) : QgsLineSegment2D(start, end)
{
    double x = end.x() - start.x(), y = end.y() - start.y();
    if ((x < 0 && y < 0) || (x > 0 && y < 0) || (x==0 && y < 0) || (y==0 && x<0))
    {
        x = -x;
        y = -y;
    }
    angle = atan2(y, x);
}
