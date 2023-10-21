#ifndef LINESEGMENT_H
#define LINESEGMENT_H

#include <qgslinesegment.h>

class LineSegment : public QgsLineSegment2D
{
public:
    LineSegment(const QgsPointXY &start, const QgsPointXY &end);

    double angle;
};


#endif // LINESEGMENT_H
