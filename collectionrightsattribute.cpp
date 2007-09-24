/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "collectionrightsattribute.h"

using namespace Akonadi;

static const char* s_accessRightsIdentifier = "AccessRights";

static Collection::Rights dataToRights( const QByteArray &data )
{
  Collection::Rights rights = Collection::NoRights;

  if ( data.isEmpty() || data.at( 0 ) == 'a' )
    return Collection::AllRights;

  for ( int i = 0; i < data.count(); ++i ) {
    switch ( data.at( i ) ) {
      case 'w': rights |= Collection::CanChangeItem; break;
      case 'c': rights |= Collection::CanCreateItem; break;
      case 'd': rights |= Collection::CanDeleteItem; break;
      case 'W': rights |= Collection::CanChangeCollection; break;
      case 'C': rights |= Collection::CanCreateCollection; break;
      case 'D': rights |= Collection::CanDeleteCollection; break;
    }
  }

  return rights;
}

static QByteArray rightsToData( Collection::Rights &rights )
{
  if ( rights == Collection::AllRights )
    return QByteArray( "a" );

  QByteArray data;
  if ( rights & Collection::CanChangeItem )
    data.append( 'w' );
  if ( rights & Collection::CanCreateItem )
    data.append( 'c' );
  if ( rights & Collection::CanDeleteItem )
    data.append( 'd' );
  if ( rights & Collection::CanChangeCollection )
    data.append( 'W' );
  if ( rights & Collection::CanCreateCollection )
    data.append( 'C' );
  if ( rights & Collection::CanDeleteCollection )
    data.append( 'D' );

  return data;
}


CollectionRightsAttribute::CollectionRightsAttribute()
  : CollectionAttribute()
{
}

CollectionRightsAttribute::~CollectionRightsAttribute()
{
}

void CollectionRightsAttribute::setRights( Collection::Rights rights )
{
  mData = rightsToData( rights );
}

Collection::Rights CollectionRightsAttribute::rights() const
{
  return dataToRights( mData );
}

CollectionRightsAttribute* CollectionRightsAttribute::clone() const
{
  return new CollectionRightsAttribute();
}

QByteArray CollectionRightsAttribute::type() const
{
  return s_accessRightsIdentifier;
}

QByteArray CollectionRightsAttribute::toByteArray() const
{
  return mData;
}

void CollectionRightsAttribute::setData( const QByteArray &data )
{
  mData = data;
}
