/***************************************************************************
                         Checkerror.cpp
                         --------
    begin                : September 2018
    copyright            : (C) 2018 by Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "checkerror.h"
#include "qgsapplication.h"

CheckError::CheckError( const Check *check,
    const QString &layerId,
    QgsFeatureId featureId,
    const QgsGeometry &geometry,
    const QgsPointXY &errorLocation,
    QgsVertexId vidx,
    const QVariant &value, ValueType valueType )
  : mCheck( check )
  , mLayerId( layerId )
  , mFeatureId( featureId )
  , mGeometry( geometry )
  , mErrorLocation( errorLocation )
  , mVidx( vidx )
  , mValue( value )
  , mValueType( valueType )
  , mStatus( StatusPending )
{
}

CheckError::CheckError( const Check *check,
    const CheckerUtils::LayerFeature &layerFeature,
    const QgsPointXY &errorLocation,
    QgsVertexId vidx,
    const QVariant &value,
    ValueType valueType )
  : mCheck( check )
  , mLayerId( layerFeature.layerId() )
  , mFeatureId( layerFeature.feature().id() )
  , mErrorLocation( errorLocation )
  , mVidx( vidx )
  , mValue( value )
  , mValueType( valueType )
  , mStatus( StatusPending )
{
  if ( vidx.part != -1 )
  {
    const QgsGeometry geom = layerFeature.geometry();
    mGeometry = QgsGeometry( CheckerUtils::getGeomPart( geom.constGet(), vidx.part )->clone() );
  }
  else
  {
    mGeometry = layerFeature.geometry();
  }
  if ( !layerFeature.useMapCrs() )
  {
    QgsVectorLayer *vl = layerFeature.layer().data();
    if ( vl )
    {
      QgsCoordinateTransform ct( vl->crs(), check->context()->mapCrs, check->context()->transformContext );
      try
      {
        mGeometry.transform( ct );
        mErrorLocation = ct.transform( mErrorLocation );
      }
      catch ( const QgsCsException & )
      {
        QgsDebugMsg( QStringLiteral( "Can not show error in current map coordinate reference system" ) );
      }
    }
  }
}

QgsGeometry CheckError::geometry() const
{
  return mGeometry;
}

QgsRectangle CheckError::contextBoundingBox() const
{
  return QgsRectangle();
}

QgsRectangle CheckError::affectedAreaBBox() const
{
  return mGeometry.boundingBox();
}

void CheckError::setFixed( int method )
{
  mStatus = StatusFixed;
  const QList<CheckResolutionMethod> methods = mCheck->availableResolutionMethods();
  for ( const CheckResolutionMethod &fix : methods )
  {
    if ( fix.id() == method )
      mResolutionMessage = fix.name();
  }
}

void CheckError::setFixFailed( const QString &reason )
{
  mStatus = StatusFixFailed;
  mResolutionMessage = reason;
}

bool CheckError::isEqual( CheckError *other ) const
{
  return other->check() == check() &&
         other->layerId() == layerId() &&
         other->featureId() == featureId() &&
         other->geometry().equals(geometry()) &&
         other->value() == value() &&
         other->vidx() == vidx();
}

bool CheckError::closeMatch( CheckError * ) const
{
  return false;
}

bool CheckError::handleChanges( const Check::Changes &changes )
{
  if ( status() == StatusObsolete )
  {
    return false;
  }

  for ( const Check::Change &change : changes.value( layerId() ).value( featureId() ) )
  {
    if ( change.what == Check::ChangeFeature )
    {
      if ( change.type == Check::ChangeRemoved )
      {
        return false;
      }
      else if ( change.type == Check::ChangeChanged )
      {
        // If the check is checking the feature at geometry nodes level, the
        // error almost certainly invalid after a geometry change. In the other
        // cases, it might likely still be valid.
        return mCheck->checkType() != Check::FeatureNodeCheck;
      }
    }
    else if ( change.what == Check::ChangePart )
    {
      if ( mVidx.part == change.vidx.part )
      {
        return false;
      }
      else if ( mVidx.part > change.vidx.part )
      {
        mVidx.part += change.type == Check::ChangeAdded ? 1 : -1;
      }
    }
    else if ( change.what == Check::ChangeRing )
    {
      if ( mVidx.partEqual( change.vidx ) )
      {
        if ( mVidx.ring == change.vidx.ring )
        {
          return false;
        }
        else if ( mVidx.ring > change.vidx.ring )
        {
          mVidx.ring += change.type == Check::ChangeAdded ? 1 : -1;
        }
      }
    }
    else if ( change.what == Check::ChangeNode )
    {
      if ( mVidx.ringEqual( change.vidx ) )
      {
        if ( mVidx.vertex == change.vidx.vertex )
        {
          return false;
        }
        else if ( mVidx.vertex > change.vidx.vertex )
        {
          mVidx.vertex += change.type == Check::ChangeAdded ? 1 : -1;
        }
      }
    }
  }
  return true;
}

QMap<QString, QgsFeatureIds> CheckError::involvedFeatures() const
{
  return QMap<QString, QSet<QgsFeatureId> >();
}

QIcon CheckError::icon() const
{
  if ( status() == CheckError::StatusFixed )
    return QgsApplication::getThemeIcon( QStringLiteral( "/algorithms/mAlgorithmCheckGeometry.svg" ) );
  else
    return QgsApplication::getThemeIcon( QStringLiteral( "/algorithms/mAlgorithmLineIntersections.svg" ) );
}

void CheckError::update( const CheckError *other )
{
  Q_ASSERT( mCheck == other->mCheck );
  Q_ASSERT( mLayerId == other->mLayerId );
  Q_ASSERT( mFeatureId == other->mFeatureId );
  mErrorLocation = other->mErrorLocation;
  mVidx = other->mVidx;
  mValue = other->mValue;
  mGeometry = other->mGeometry;
}

Check::LayerFeatureIds::LayerFeatureIds( const QMap<QString, QgsFeatureIds> &idsIn )
  : ids( idsIn )
{
}


void SingleCheckError::update( const SingleCheckError *other )
{
  Q_ASSERT( mCheck == other->mCheck );
  mErrorLocation = other->mErrorLocation;
  mVertexId = other->mVertexId;
  mGeometry = other->mGeometry;
}

bool SingleCheckError::isEqual( const SingleCheckError *other ) const
{
  return mGeometry.equals( other->mGeometry )
         && mCheck == other->mCheck
         && mErrorLocation.equals( other->mErrorLocation )
         && mVertexId == other->mVertexId;
}

bool SingleCheckError::handleChanges( const QList<Check::Change> &changes )
{
  Q_UNUSED( changes )
  return true;
}

QString SingleCheckError::description() const
{
  return mCheck->description();
}

const Check *SingleCheckError::check() const
{
  return mCheck;
}

QgsGeometry SingleCheckError::errorLocation() const
{
  return mErrorLocation;
}

QgsVertexId SingleCheckError::vertexId() const
{
  return mVertexId;
}

CheckErrorSingle::CheckErrorSingle( SingleCheckError *error, const CheckerUtils::LayerFeature &layerFeature )
    : CheckError( error->check(), layerFeature, QgsPointXY( error->errorLocation().constGet()->centroid() ), error->vertexId() ) // TODO: should send geometry to CheckError
    , mError( error )
{
}

SingleCheckError *CheckErrorSingle::singleError() const
{
  return mError;
}

bool CheckErrorSingle::handleChanges( const Check::Changes &changes )
{
  if ( !CheckError::handleChanges( changes ) )
    return false;

  return mError->handleChanges( changes.value( layerId() ).value( featureId() ) );
}
