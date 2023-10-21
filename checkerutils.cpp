/***************************************************************************
 *  CheckerUtils.cpp                                            *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "checkcontext.h"
#include "checkerutils.h"
#include "qgsgeometry.h"
#include "qgsgeometryutils.h"
#include "featurepool.h"
#include "qgspolygon.h"
#include "qgsgeos.h"
#include "qgsgeometrycollection.h"
#include "qgssurface.h"
#include "qgsvectorlayer.h"
#include "check.h"
#include "qgsfeedback.h"

#include <qmath.h>

CheckerUtils::LayerFeature::LayerFeature( const FeaturePool *pool,
    const QgsFeature &feature,
    const CheckContext *context,
    bool useMapCrs )
  : mFeaturePool( pool )
  , mFeature( feature )
  , mMapCrs( useMapCrs )
{
  mGeometry = feature.geometry();
  const QgsCoordinateTransform transform( pool->crs(), context->mapCrs, context->transformContext );
  if ( useMapCrs && context->mapCrs.isValid() && !transform.isShortCircuited() )
  {
    try
    {
      mGeometry.transform( transform );
    }
    catch ( const QgsCsException & )
    {
      QgsDebugMsg( QStringLiteral( "Shrug. What shall we do with a geometry that cannot be converted?" ) );
    }
  }
}

QgsFeature CheckerUtils::LayerFeature::feature() const
{
  return mFeature;
}

QPointer<QgsVectorLayer> CheckerUtils::LayerFeature::layer() const
{
  return mFeaturePool->layerPtr();
}

QString CheckerUtils::LayerFeature::layerId() const
{
  return mFeaturePool->layerId();
}

QgsGeometry CheckerUtils::LayerFeature::geometry() const
{
  return mGeometry;
}

QString CheckerUtils::LayerFeature::id() const
{
  return QStringLiteral( "%1:%2" ).arg( mFeaturePool->layerName() ).arg( mFeature.id() );
}

bool CheckerUtils::LayerFeature::operator==( const LayerFeature &other ) const
{
  return layerId() == other.layerId() && mFeature.id() == other.mFeature.id();
}

bool CheckerUtils::LayerFeature::operator!=( const LayerFeature &other ) const
{
  return layerId() != other.layerId() || mFeature.id() != other.mFeature.id();
}

/////////////////////////////////////////////////////////////////////////////

CheckerUtils::LayerFeatures::iterator::iterator( const QStringList::const_iterator &layerIt, const LayerFeatures *parent )
  : mLayerIt( layerIt )
  , mFeatureIt( QgsFeatureIds::const_iterator() )
  , mParent( parent )
{
  nextLayerFeature( true );
}

CheckerUtils::LayerFeatures::iterator::iterator( const CheckerUtils::LayerFeatures::iterator &rh )
{
  mLayerIt = rh.mLayerIt;
  mFeatureIt = rh.mFeatureIt;
  mParent = rh.mParent;
  mCurrentFeature = qgis::make_unique<LayerFeature>( *rh.mCurrentFeature.get() );
}

bool CheckerUtils::LayerFeature::useMapCrs() const
{
  return mMapCrs;
}
CheckerUtils::LayerFeatures::iterator::~iterator()
{
}

CheckerUtils::LayerFeatures::iterator CheckerUtils::LayerFeatures::iterator::operator++( int )
{
  iterator tmp( *this );
  ++*this;
  return tmp;
}

const CheckerUtils::LayerFeature &CheckerUtils::LayerFeatures::iterator::operator*() const
{
  Q_ASSERT( mCurrentFeature );
  return *mCurrentFeature;
}

bool CheckerUtils::LayerFeatures::iterator::operator!=( const CheckerUtils::LayerFeatures::iterator &other )
{
  return mLayerIt != other.mLayerIt || mFeatureIt != other.mFeatureIt;
}

const CheckerUtils::LayerFeatures::iterator &CheckerUtils::LayerFeatures::iterator::operator++()
{
  nextLayerFeature( false );
  return *this;
}
bool CheckerUtils::LayerFeatures::iterator::nextLayerFeature( bool begin )
{
  if ( !begin && nextFeature( false ) )
  {
    return true;
  }
  while ( nextLayer( begin ) )
  {
    begin = false;
    if ( nextFeature( true ) )
    {
      return true;
    }
  }
  // End
  mFeatureIt = QgsFeatureIds::const_iterator();
  mCurrentFeature.reset();
  return false;
}

bool CheckerUtils::LayerFeatures::iterator::nextLayer( bool begin )
{
  if ( !begin )
  {
    ++mLayerIt;
  }
  while ( true )
  {
    if ( mLayerIt == mParent->mLayerIds.end() )
    {
      break;
    }
    if ( mParent->mGeometryTypes.contains( mParent->mFeaturePools[*mLayerIt]->geometryType() ) )
    {
      mFeatureIt = mParent->mFeatureIds[*mLayerIt].constBegin();
      return true;
    }
    ++mLayerIt;
  }
  return false;
}

bool CheckerUtils::LayerFeatures::iterator::nextFeature( bool begin )
{
  FeaturePool *featurePool = mParent->mFeaturePools[*mLayerIt];
  const QgsFeatureIds &featureIds = mParent->mFeatureIds[*mLayerIt];
  if ( !begin )
  {
    ++mFeatureIt;
  }
  while ( true )
  {
    if ( mFeatureIt == featureIds.end() )
    {
      break;
    }
    if ( mParent->mFeedback )
      mParent->mFeedback->setProgress( mParent->mFeedback->progress() + 1.0 );
    QgsFeature feature;
    if ( featurePool->getFeature( *mFeatureIt, feature ) && !feature.geometry().isNull() )
    {
      mCurrentFeature = qgis::make_unique<LayerFeature>( featurePool, feature, mParent->mContext, mParent->mUseMapCrs );
      return true;
    }
    ++mFeatureIt;
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////

CheckerUtils::LayerFeatures::LayerFeatures( const QMap<QString, FeaturePool *> &featurePools,
    const QMap<QString, QgsFeatureIds> &featureIds,
    const QList<QgsWkbTypes::GeometryType> &geometryTypes,
    QgsFeedback *feedback,
    const CheckContext *context,
    bool useMapCrs )
  : mFeaturePools( featurePools )
  , mFeatureIds( featureIds )
  , mLayerIds( featurePools.keys() )
  , mGeometryTypes( geometryTypes )
  , mFeedback( feedback )
  , mContext( context )
  , mUseMapCrs( useMapCrs )
{}

CheckerUtils::LayerFeatures::LayerFeatures( const QMap<QString, FeaturePool *> &featurePools,
    const QList<QString> &layerIds, const QgsRectangle &extent,
    const QList<QgsWkbTypes::GeometryType> &geometryTypes,
    const CheckContext *context )
  : mFeaturePools( featurePools )
  , mLayerIds( layerIds )
  , mExtent( extent )
  , mGeometryTypes( geometryTypes )
  , mContext( context )
  , mUseMapCrs( true )
{
  for ( const QString &layerId : layerIds )
  {
    const FeaturePool *featurePool = featurePools[layerId];
    if ( geometryTypes.contains( featurePool->geometryType() ) )
    {
      QgsCoordinateTransform ct( featurePool->crs(), context->mapCrs, context->transformContext );
      mFeatureIds.insert( layerId, featurePool->getIntersects( ct.transform( extent, QgsCoordinateTransform::ReverseTransform ) ) );
    }
    else
    {
      mFeatureIds.insert( layerId, QgsFeatureIds() );
    }
  }
}

CheckerUtils::LayerFeatures::iterator CheckerUtils::LayerFeatures::begin() const
{
  return iterator( mLayerIds.constBegin(), this );
}

CheckerUtils::LayerFeatures::iterator CheckerUtils::LayerFeatures::end() const
{
  return iterator( mLayerIds.end(), this );
}

/////////////////////////////////////////////////////////////////////////////

std::unique_ptr<QgsGeometryEngine> CheckerUtils::createGeomEngine( const QgsAbstractGeometry *geometry, double tolerance )
{
  return qgis::make_unique<QgsGeos>( geometry, tolerance );
}

QgsAbstractGeometry *CheckerUtils::getGeomPart( QgsAbstractGeometry *geom, int partIdx )
{
  if ( dynamic_cast<QgsGeometryCollection *>( geom ) )
  {
    return static_cast<QgsGeometryCollection *>( geom )->geometryN( partIdx );
  }
  return geom;
}

const QgsAbstractGeometry *CheckerUtils::getGeomPart( const QgsAbstractGeometry *geom, int partIdx )
{
  if ( dynamic_cast<const QgsGeometryCollection *>( geom ) )
  {
    return static_cast<const QgsGeometryCollection *>( geom )->geometryN( partIdx );
  }
  return geom;
}

QList<const QgsLineString *> CheckerUtils::polygonRings( const QgsPolygon *polygon )
{
  QList<const QgsLineString *> rings;
  if ( const QgsLineString *exterior = dynamic_cast<const QgsLineString *>( polygon->exteriorRing() ) )
  {
    rings.append( exterior );
  }
  for ( int iInt = 0, nInt = polygon->numInteriorRings(); iInt < nInt; ++iInt )
  {
    if ( const QgsLineString *interior = dynamic_cast<const QgsLineString *>( polygon->interiorRing( iInt ) ) )
    {
      rings.append( interior );
    }
  }
  return rings;
}

void CheckerUtils::filter1DTypes( QgsAbstractGeometry *geom )
{
  if ( qgsgeometry_cast<QgsGeometryCollection *>( geom ) )
  {
    QgsGeometryCollection *geomCollection = static_cast<QgsGeometryCollection *>( geom );
    for ( int nParts = geom->partCount(), iPart = nParts - 1; iPart >= 0; --iPart )
    {
      if ( !qgsgeometry_cast<QgsSurface *>( geomCollection->geometryN( iPart ) ) )
      {
        geomCollection->removeGeometry( iPart );
      }
    }
  }
}

bool CheckerUtils::pointOnLine(const QgsPointXY &M, const QgsPointXY &A, const QgsPointXY &B, double tol)
{
  if (M.distance(A) < tol || M.distance(B) < tol)
    return true;

  if (abs(A.x() - B.x()) < tol && abs(A.x() - M.x()) < tol)
  {
    if ((A.y() - M.y()) * (M.y() - B.y()) > 0)
      return true;
    return false;
  }
  else if (abs(A.y() - B.y()) < tol && abs(A.y() - M.y()) < tol)
  {
    if ((A.x() - M.x()) * (M.x() - B.x()) > 0)
      return true;
    return false;
  }
  else
  {
    if (abs((A.y() - M.y()) / (A.x() - M.x()) -
            (M.y() - B.y()) / (M.x() - B.x())) < tol)
    {
      if ((A.y() - M.y()) * (M.y() - B.y()) > 0 &&
          (A.x() - M.x()) * (M.x() - B.x()) > 0)
        return true;
    }

    return false;
  }
}


QgsPolylineXY CheckerUtils::lineOverlay(LineSegment &a, LineSegment &b, double tol)
{
  QgsPointXY A = a.start(), B = a.end(), C = b.start(), D = b.end();
  bool onLineA, onLineB, onLineC, onLineD;
  onLineA = pointOnLine(A, C, D, tol);
  onLineB = pointOnLine(B, C, D, tol);
  onLineC = pointOnLine(C, A, B, tol);
  onLineD = pointOnLine(D, A, B, tol);

  QgsPolylineXY overlapLine;
  // coincide01
  if (onLineA && onLineB)
  {
    overlapLine.push_back(A);
    overlapLine.push_back(B);
  }
  else if (onLineC && onLineD)
  {
    overlapLine.push_back(C);
    overlapLine.push_back(D);
  }

  // coincide02
  if (onLineA && onLineC && (A != C))
  {
    overlapLine.push_back(A);
    overlapLine.push_back(C);
  }
  else if (onLineA && onLineD && (A != D))
  {
    overlapLine.push_back(A);
    overlapLine.push_back(D);
  }
  else if (onLineB && onLineC && (B != C))
  {
    overlapLine.push_back(B);
    overlapLine.push_back(C);
  }
  else if (onLineB && onLineD && (B != D))
  {
    overlapLine.push_back(B);
    overlapLine.push_back(D);
  }

  return overlapLine;
}

QVector<QgsGeometry> CheckerUtils::lineOverlay(QVector<LineSegment> &linesA, QVector<LineSegment> &linesB, double tol)
{
  QVector<QgsGeometry> ans;
  auto i = linesA.begin();
  auto j = linesB.begin();
  for (; i < linesA.end() && j < linesB.end(); ++i)
  {
    while (j < linesB.end() && j->angle + tol < i->angle)
      ++j;
    if (j >= linesB.end())
      break;
    if (j->angle > i->angle + tol)
      continue;

    auto k = j;
    while (k < linesB.end() && ok(i->angle, k->angle, tol))
    {
      QgsPolylineXY line = lineOverlay(*i, *k, tol);
      if (!line.isEmpty())
      {
        ans.push_back(QgsGeometry::fromPolylineXY(line));
      }

      ++k;
    }
  }
  return ans;
}

double pointLineDist( const QgsPoint &p1, const QgsPoint &p2, const QgsPoint &q )
{
  double nom = std::fabs( ( p2.y() - p1.y() ) * q.x() - ( p2.x() - p1.x() ) * q.y() + p2.x() * p1.y() - p2.y() * p1.x() );
  double dx = p2.x() - p1.x();
  double dy = p2.y() - p1.y();
  return nom / std::sqrt( dx * dx + dy * dy );
}

bool CheckerUtils::pointOnLine( const QgsPoint &p, const QgsLineString *line, double tol, bool excludeExtremities )
{
  int nVerts = line->vertexCount();
  for ( int i = 0 + excludeExtremities; i < nVerts - 1 - excludeExtremities; ++i )
  {
    QgsPoint p1 = line->vertexAt( QgsVertexId( 0, 0, i ) );
    QgsPoint p2 = line->vertexAt( QgsVertexId( 0, 0, i + 1 ) );
    double dist = pointLineDist( p1, p2, p );
    if ( dist < tol )
    {
      return true;
    }
  }
  return false;
}

QVector<CheckerUtils::SelfIntersection> CheckerUtils::selfIntersections(const QgsAbstractGeometry *geom, int part, int ring, double tolerance, bool acceptImproperIntersection)
{
  QVector<SelfIntersection> intersections;

  int n = geom->vertexCount( part, ring );
  bool isClosed = geom->vertexAt( QgsVertexId( part, ring, 0 ) ) == geom->vertexAt( QgsVertexId( part, ring, n - 1 ) );
  int existI = -2, existJ = -2;
  // Check every pair of segments for intersections
  for ( int i = 0, j = 1; j < n; i = j++ )
  {
    QgsPoint pi = geom->vertexAt( QgsVertexId( part, ring, i ) );
    QgsPoint pj = geom->vertexAt( QgsVertexId( part, ring, j ) );
    if ( QgsGeometryUtils::sqrDistance2D( pi, pj ) < tolerance * tolerance ) continue;

    // Don't test neighboring edges
    int start = j + 1;
    int end = i == 0 && isClosed ? n - 1 : n;
    for ( int k = start, l = start + 1; l < end; k = l++ )
    {
      QgsPoint pk = geom->vertexAt( QgsVertexId( part, ring, k ) );
      QgsPoint pl = geom->vertexAt( QgsVertexId( part, ring, l ) );

      QgsPoint inter;
      bool intersection = false;
      if ( !QgsGeometryUtils::segmentIntersection( pi, pj, pk, pl, inter, intersection, tolerance, acceptImproperIntersection ) ) continue;

      if (inter.distance(pi) <= tolerance && existI == i) {
        continue;
      } else if (inter.distance(pk) <= tolerance && existJ == k) {
        continue;
      } else if (inter.distance(pj) <= tolerance || inter.distance(pl) <= tolerance) {
        SelfIntersection s;
        s.segment1 = i;
        s.segment2 = k;
        if ( s.segment1 > s.segment2 )
        {
            std::swap( s.segment1, s.segment2 );
        }
        s.point = inter;
        intersections.append( s );
        if (inter.distance(pj) <= tolerance)
            existI = i + 1;
        if (inter.distance(pl) <= tolerance)
            existJ = k + 1;
      } else {
        SelfIntersection s;
        s.segment1 = i;
        s.segment2 = k;
        if ( s.segment1 > s.segment2 )
        {
            std::swap( s.segment1, s.segment2 );
        }
        s.point = inter;
        intersections.append( s );
      }
    }
  }
  return intersections;

}

QList<QgsPoint> CheckerUtils::lineIntersections( const QgsLineString *line1, const QgsLineString *line2, double tol, bool acceptImproperIntersection )
{
  QList<QgsPoint> intersections;
  QgsPoint inter;
  bool intersection = false;
  int existI = -2, existJ = -2;
  for (int i = 0, n = line1->vertexCount() - 1; i < n; ++i)
  {
    for ( int j = 0, m = line2->vertexCount() - 1; j < m; ++j )
    {
      QgsPoint p1 = line1->vertexAt( QgsVertexId( 0, 0, i ) );
      QgsPoint p2 = line1->vertexAt( QgsVertexId( 0, 0, i + 1 ) );
      QgsPoint q1 = line2->vertexAt( QgsVertexId( 0, 0, j ) );
      QgsPoint q2 = line2->vertexAt( QgsVertexId( 0, 0, j + 1 ) );
      if ( QgsGeometryUtils::segmentIntersection( p1, p2, q1, q2, inter, intersection, tol, acceptImproperIntersection ) )
      {
        if (inter.distance(p1) <= tol && existI == i) {
            continue;
        } else if (inter.distance(q1) <= tol && existJ == j) {
            continue;
        } else if (inter.distance(p2) <= tol || inter.distance(q2) <= tol) {
            intersections.append(inter);
            if (inter.distance(p2) <= tol)
                existI = i + 1;
            if (inter.distance(q2) <= tol)
                existJ = j + 1;
        } else {
            intersections.append(inter);
        }
      }
    }
  }
  return intersections;
}

double CheckerUtils::sharedEdgeLength( const QgsAbstractGeometry *geom1, const QgsAbstractGeometry *geom2, double tol )
{
  double len = 0;

  // Test every pair of segments for shared edges
  for ( int iPart1 = 0, nParts1 = geom1->partCount(); iPart1 < nParts1; ++iPart1 )
  {
    for ( int iRing1 = 0, nRings1 = geom1->ringCount( iPart1 ); iRing1 < nRings1; ++iRing1 )
    {
      for ( int iVert1 = 0, jVert1 = 1, nVerts1 = geom1->vertexCount( iPart1, iRing1 ); jVert1 < nVerts1; iVert1 = jVert1++ )
      {
        QgsPoint p1 = geom1->vertexAt( QgsVertexId( iPart1, iRing1, iVert1 ) );
        QgsPoint p2 = geom1->vertexAt( QgsVertexId( iPart1, iRing1, jVert1 ) );
        double lambdap1 = 0.;
        double lambdap2 = std::sqrt( QgsGeometryUtils::sqrDistance2D( p1, p2 ) );
        QgsVector d;
        try
        {
          d = QgsVector( p2.x() - p1.x(), p2.y() - p1.y() ).normalized();
        }
        catch ( const QgsException & )
        {
          // Edge has zero length, skip
          continue;
        }

        for ( int iPart2 = 0, nParts2 = geom2->partCount(); iPart2 < nParts2; ++iPart2 )
        {
          for ( int iRing2 = 0, nRings2 = geom2->ringCount( iPart2 ); iRing2 < nRings2; ++iRing2 )
          {
            for ( int iVert2 = 0, jVert2 = 1, nVerts2 = geom2->vertexCount( iPart2, iRing2 ); jVert2 < nVerts2; iVert2 = jVert2++ )
            {
              QgsPoint q1 = geom2->vertexAt( QgsVertexId( iPart2, iRing2, iVert2 ) );
              QgsPoint q2 = geom2->vertexAt( QgsVertexId( iPart2, iRing2, jVert2 ) );

              // Check whether q1 and q2 are on the line p1, p
              if ( pointLineDist( p1, p2, q1 ) <= tol && pointLineDist( p1, p2, q2 ) <= tol )
              {
                // Get length common edge
                double lambdaq1 = QgsVector( q1.x() - p1.x(), q1.y() - p1.y() ) * d;
                double lambdaq2 = QgsVector( q2.x() - p1.x(), q2.y() - p1.y() ) * d;
                if ( lambdaq1 > lambdaq2 )
                {
                  std::swap( lambdaq1, lambdaq2 );
                }
                double lambda1 = std::max( lambdaq1, lambdap1 );
                double lambda2 = std::min( lambdaq2, lambdap2 );
                len += std::max( 0., lambda2 - lambda1 );
              }
            }
          }
        }
      }
    }
  }
  return len;
}
