/***************************************************************************
                      qgsvectorlayerfeaturepool.h
                     --------------------------------------
Date                 : 18.9.2018
Copyright            : (C) 2018 by Matthias Kuhn
email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "vectorlayerfeaturepool.h"
#include "qgsthreadingutils.h"

#include "qgsfeaturerequest.h"
#include "qgsvectorlayer.h"

VectorLayerFeaturePool::VectorLayerFeaturePool( QgsVectorLayer *layer )
  : QObject()
  , FeaturePool( layer )
{
  connect( layer, &QgsVectorLayer::featureDeleted, this, &VectorLayerFeaturePool::onFeatureDeleted );
  connect( layer, &QgsVectorLayer::geometryChanged, this, &VectorLayerFeaturePool::onGeometryChanged );
}

bool VectorLayerFeaturePool::addFeature( QgsFeature &feature, Flags flags )
{
  Q_UNUSED( flags )

  bool res = false;

  auto addFeatureSynchronized = [ this, &feature, &res ]()
  {
    QgsVectorLayer *lyr = layer();
    if ( lyr )
      res = lyr->addFeature( feature );
  };

  QgsThreadingUtils::runOnMainThread( addFeatureSynchronized );

  if ( !res )
    return false;

#if 0
  if ( mSelectedOnly )
  {
    QgsThreadingUtils::runOnMainThread( [ this, feature ]()
    {
      QgsVectorLayer *lyr = layer();
      if ( lyr )
      {
        QgsFeatureIds selectedFeatureIds = lyr->selectedFeatureIds();
        selectedFeatureIds.insert( feature.id() );
        lyr->selectByIds( selectedFeatureIds );
      }
    } );
  }
#endif
  insertFeature( feature );

  return res;
}

bool VectorLayerFeaturePool::addFeatures( QgsFeatureList &features, QgsFeatureSink::Flags flags )
{
  Q_UNUSED( flags )

  bool res = false;

  auto addFeatureSynchronized = [ this, &features, &res ]()
  {
    QgsVectorLayer *lyr = layer();
    if ( lyr )
      res = lyr->addFeatures( features );
  };

  QgsThreadingUtils::runOnMainThread( addFeatureSynchronized );

  if ( !res )
    return false;

#if 0
  if ( mSelectedOnly )
  {
    QgsThreadingUtils::runOnMainThread( [ this, features ]()
    {
      QgsVectorLayer *lyr = layer();
      if ( lyr )
      {
        QgsFeatureIds selectedFeatureIds = lyr->selectedFeatureIds();
        for ( const QgsFeature &feature : qgis::as_const( features ) )
          selectedFeatureIds.insert( feature.id() );
        lyr->selectByIds( selectedFeatureIds );
      }
    } );
  }
#endif

  for ( const QgsFeature &feature : qgis::as_const( features ) )
    insertFeature( feature );

  return res;
}

void VectorLayerFeaturePool::updateFeature( QgsFeature &feature )
{
  QgsThreadingUtils::runOnMainThread( [this, &feature]()
  {
    QgsVectorLayer *lyr = layer();
    if ( lyr )
    {
      lyr->updateFeature( feature );
    }
  } );

  refreshCache( feature );
}

void VectorLayerFeaturePool::deleteFeature( QgsFeatureId fid )
{
  removeFeature( fid );
  QgsThreadingUtils::runOnMainThread( [this, fid]()
  {
    QgsVectorLayer *lyr = layer();
    if ( lyr )
    {
      lyr->deleteFeatures( QgsFeatureIds() << fid );
    }
  } );
}

void VectorLayerFeaturePool::onGeometryChanged( QgsFeatureId fid, const QgsGeometry &geometry )
{
  Q_UNUSED( geometry )

  if ( isFeatureCached( fid ) )
  {
    QgsFeature feature;
    getFeature( fid, feature );
    refreshCache( feature );
  }
}

void VectorLayerFeaturePool::onFeatureDeleted( QgsFeatureId fid )
{
  deleteFeature( fid );
}
