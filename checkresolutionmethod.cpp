/***************************************************************************
    qgsgeometrycheckresolutionmethod.cpp
     --------------------------------------
    Date                 : January 2020
    Copyright            : (C) 2020 Matthias Kuhn
    Email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "checkresolutionmethod.h"

CheckResolutionMethod::CheckResolutionMethod( int id, const QString &name, const QString &description, bool isStable )
  : mId( id )
  , mIsStable( isStable )
  , mName( name )
  , mDescription( description )
{
}

int CheckResolutionMethod::id() const
{
  return mId;
}

bool CheckResolutionMethod::isStable() const
{
  return mIsStable;
}

QString CheckResolutionMethod::name() const
{
  return mName;
}

QString CheckResolutionMethod::description() const
{
  return mDescription;
}
