/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTION_P_H
#define AKONADI_COLLECTION_P_H

#include "collection.h"
#include "entity_p.h"

using namespace Akonadi;

class Akonadi::CollectionPrivate : public EntityPrivate
{
  public:
    CollectionPrivate( Collection::Id id = -1 ) :
      EntityPrivate( id ),
      parentId( -1 ),
      contentTypesChanged( false ),
      cachePolicyChanged( false )
    {}

    CollectionPrivate( const CollectionPrivate &other ) :
      EntityPrivate( other )
    {
      parentId = other.parentId;
      name = other.name;
      parentRemoteId = other.parentRemoteId;
      resource = other.resource;
      statistics = other.statistics;
      contentTypes = other.contentTypes;
      cachePolicy = other.cachePolicy;
      contentTypesChanged = other.contentTypesChanged;
      cachePolicyChanged = other.cachePolicyChanged;
    }

    ~CollectionPrivate()
    {
    }

    CollectionPrivate* clone() const
    {
      return new CollectionPrivate( *this );
    }

    void resetChangeLog()
    {
      contentTypesChanged = false;
      cachePolicyChanged = false;
      EntityPrivate::resetChangeLog();
    }

    static Collection newRoot()
    {
      Collection root( 0 );
      QStringList types;
      types << Collection::mimeType();
      root.setContentMimeTypes( types );
      return root;
    }

    Collection::Id parentId;
    QString name;
    QString parentRemoteId;
    QString resource;
    CollectionStatistics statistics;
    QStringList contentTypes;
    static const Collection root;
    CachePolicy cachePolicy;
    bool contentTypesChanged;
    bool cachePolicyChanged;
};

#endif
