/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#include <QtCore/QDebug>
#include <QtCore/QUrl>

#include "nepomukmanager.h"

#include "search/result.h"
#include "search/query.h"

#include "entities.h"

using namespace Akonadi;

static qint64 uriToItemId( const QUrl &url )
{
  bool ok = false;

  const qint64 id = url.queryItemValue( QLatin1String( "item" ) ).toLongLong( &ok );

  if ( !ok )
    return -1;
  else
    return id;
}

NepomukManager::NepomukManager( QObject* parent )
  : QObject( parent ),
    mValid( true )
{
  Q_ASSERT( mInstance == 0 );
  mInstance = this;

  if ( !Nepomuk::Search::QueryServiceClient::serviceAvailable() ) {
    qWarning() << "Nepomuk QueryServer interface not available!";
    mValid = false;
  } else {
    reloadSearches();
  }
}

NepomukManager::~NepomukManager()
{
  if ( mValid )
    stopSearches();
}

bool NepomukManager::addSearch( const Collection &collection )
{
  if ( !mValid )
    return false;

  QMutexLocker lock( &mMutex );
  if ( collection.remoteId().isEmpty() )
    return false;

  // TODO: search statement is stored in remoteId currently, port to attributes!
  const QString searchStatement = collection.remoteId();

  Nepomuk::Search::QueryServiceClient *query = new Nepomuk::Search::QueryServiceClient( this );

  connect( query, SIGNAL( newEntries( const QList<Nepomuk::Search::Result>& ) ),
           this, SLOT( hitsAdded( const QList<Nepomuk::Search::Result>& ) ) );
  connect( query, SIGNAL( entriesRemoved( const QList<QUrl>& ) ),
           this, SLOT( hitsRemoved( const QList<QUrl>& ) ) );

  mQueryMap.insert( query, collection.id() );
  mQueryInvMap.insert( collection.id(), query ); // needed for fast lookup in removeSearch()

  // query with SPARQL statement
  query->query( Nepomuk::Search::Query( searchStatement ) );

  return true;
}

bool NepomukManager::removeSearch( qint64 collectionId )
{
  Nepomuk::Search::QueryServiceClient *query = mQueryInvMap.value( collectionId );
  if ( !query ) {
    qWarning() << "Nepomuk QueryServer: Query could not be removed!";
    return false;
  } else {
    query->close();
  }

  // cleanup mappings
  mQueryInvMap.remove( collectionId );
  mQueryMap.remove( query );

  query->deleteLater();

  return true;
}

void NepomukManager::reloadSearches()
{
  Resource resource = Resource::retrieveByName( QLatin1String( "akonadi_search_resource" ) );
  if ( !resource.isValid() ) {
    qWarning() << "Nepomuk QueryServer: No valid search resource found!";
    return;
  }

  const Collection::List collections = resource.collections();
  Q_FOREACH ( const Collection &collection, collections ) {
    addSearch( collection );
  }
}

void NepomukManager::stopSearches()
{
  Resource resource = Resource::retrieveByName( QLatin1String( "akonadi_search_resource" ) );
  if ( !resource.isValid() ) {
    qWarning() << "Nepomuk QueryServer: No valid search resource found!";
    return;
  }

  const Collection::List collections = resource.collections();
  Q_FOREACH ( const Collection &collection, collections ) {
    removeSearch( collection.id() );
  }
}

void NepomukManager::hitsAdded( const QList<Nepomuk::Search::Result>& entries )
{
  Nepomuk::Search::QueryServiceClient *query = qobject_cast<Nepomuk::Search::QueryServiceClient*>( sender() );
  if ( !query ) {
    qWarning() << "Nepomuk QueryServer: Got signal from non-existing search query!";
    return;
  }

  mMutex.lock();
  qint64 collectionId = mQueryMap.value( query );
  mMutex.unlock();

  Q_FOREACH( const Nepomuk::Search::Result &result, entries ) {
    const qint64 itemId = uriToItemId( result.resourceUri() );

    if ( itemId == -1 ) {
      qWarning() << "Nepomuk QueryServer: Retrieved invalid item id from server!";
      continue;
    }

    Entity::addToRelation<CollectionPimItemRelation>( collectionId, itemId );
  }
}

void NepomukManager::hitsRemoved( const QList<QUrl> &entries )
{
  Nepomuk::Search::QueryServiceClient *query = qobject_cast<Nepomuk::Search::QueryServiceClient*>( sender() );
  if ( !query ) {
    qWarning() << "Nepomuk QueryServer: Got signal from non-existing search query!";
    return;
  }

  mMutex.lock();
  qint64 collectionId = mQueryMap.value( query );
  mMutex.unlock();

  Q_FOREACH( const QUrl &uri, entries ) {
    const qint64 itemId = uriToItemId( uri );

    if ( itemId == -1 ) {
      qWarning() << "Nepomuk QueryServer: Retrieved invalid item id from server!";
      continue;
    }

    Entity::removeFromRelation<CollectionPimItemRelation>( collectionId, itemId );
  }
}

#include "nepomukmanager.moc"
