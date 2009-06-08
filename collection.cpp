/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collection.h"
#include "collection_p.h"

#include "attributefactory.h"
#include "cachepolicy.h"
#include "collectionrightsattribute_p.h"
#include "collectionstatistics.h"
#include "entity_p.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <KUrl>
#include <KGlobal>

using namespace Akonadi;

class CollectionRoot : public Collection
{
  public:
    CollectionRoot()
      : Collection( 0 )
    {
      QStringList types;
      types << Collection::mimeType();
      setContentMimeTypes( types );

      // The root collection is read-only for the users
      Collection::Rights rights;
      rights |= Collection::ReadOnly;
      setRights( rights );
    }
};

K_GLOBAL_STATIC( CollectionRoot, s_root )

Collection::Collection() :
    Entity( new CollectionPrivate )
{
  Q_D( Collection );
  static int lastId = -1;
  d->mId = lastId--;
}

Collection::Collection( Id id ) :
    Entity( new CollectionPrivate( id ) )
{
}

Collection::Collection(const Collection & other) :
    Entity( other )
{
}

Collection::~Collection()
{
}

QString Collection::name( ) const
{
  return d_func()->name;
}

void Collection::setName( const QString & name )
{
  Q_D( Collection );
  d->name = name;
}

Collection::Rights Collection::rights() const
{
  CollectionRightsAttribute *attr = attribute<CollectionRightsAttribute>();
  if ( attr )
    return attr->rights();
  else
    return AllRights;
}

void Collection::setRights( Rights rights )
{
  CollectionRightsAttribute *attr = attribute<CollectionRightsAttribute>( AddIfMissing );
  attr->setRights( rights );
}

QStringList Collection::contentMimeTypes() const
{
  return d_func()->contentTypes;
}

void Collection::setContentMimeTypes( const QStringList & types )
{
  Q_D( Collection );
  d->contentTypes = types;
  d->contentTypesChanged = true;
}

Collection::Id Collection::parent() const
{
  return d_func()->parentId;
}

void Collection::setParent( Id parent )
{
  Q_D( Collection );
  d->parentId = parent;
}

void Collection::setParent(const Collection & collection)
{
  Q_D( Collection );
  d->parentId = collection.id();
  d->parentRemoteId = collection.remoteId();
}

QString Collection::parentRemoteId() const
{
  return d_func()->parentRemoteId;
}

void Collection::setParentRemoteId(const QString & remoteParent)
{
  Q_D( Collection );
  d->parentRemoteId = remoteParent;
}

KUrl Collection::url() const
{
  KUrl url;
  url.setProtocol( QString::fromLatin1("akonadi") );
  url.addQueryItem( QLatin1String("collection"), QString::number( id() ) );
  return url;
}

Collection Collection::fromUrl( const KUrl &url )
{
  if ( url.protocol() != QLatin1String( "akonadi" ) )
    return Collection();

  const QString colStr = url.queryItem( QLatin1String( "collection" ) );
  bool ok = false;
  Collection::Id colId = colStr.toLongLong( &ok );
  if ( !ok )
    return Collection();

  if ( colId == 0 )
    return Collection::root();

  return Collection( colId );
}

Collection Collection::root()
{
  return *s_root;
}

QString Collection::mimeType( )
{
  return QString::fromLatin1("inode/directory");
}

QString Collection::resource() const
{
  return d_func()->resource;
}

void Collection::setResource(const QString & resource)
{
  Q_D( Collection );
  d->resource = resource;
}

uint qHash( const Akonadi::Collection &collection )
{
  return qHash( collection.id() );
}

CollectionStatistics Collection::statistics() const
{
  return d_func()->statistics;
}

void Collection::setStatistics(const CollectionStatistics & statistics)
{
  Q_D( Collection );
  d->statistics = statistics;
}

CachePolicy Collection::cachePolicy() const
{
  return d_func()->cachePolicy;
}

void Collection::setCachePolicy(const CachePolicy & cachePolicy)
{
  Q_D( Collection );
  d->cachePolicy = cachePolicy;
  d->cachePolicyChanged = true;
}

AKONADI_DEFINE_PRIVATE( Akonadi::Collection )
