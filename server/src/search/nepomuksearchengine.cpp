/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "nepomuksearchengine.h"

#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QMetaObject>

#include "nepomuk/result.h"

#include "entities.h"
#include "storage/notificationcollector.h"
#include "storage/selectquerybuilder.h"
#include "notificationmanager.h"

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

NepomukSearchEngine::NepomukSearchEngine( QObject* parent )
  : QObject( parent ),
    mValid( true ),
    mCollector( new NotificationCollector( this ) )
{
  NotificationManager::self()->connectNotificationCollector( mCollector );

  // FIXME: service availability might change, and we probably should attempt something to change it
  if ( !Nepomuk::Search::QueryServiceClient::serviceAvailable() ) {
    qWarning() << "Nepomuk QueryServer interface not available!";
    mValid = false;
  } else {
    reloadSearches();
  }
}

NepomukSearchEngine::~NepomukSearchEngine()
{
  if ( mValid )
    stopSearches();
}

void NepomukSearchEngine::addSearch( const Collection &collection )
{
  if ( !mValid || collection.queryLanguage() != QLatin1String( "SPARQL" ) )
    return;

  Nepomuk::Search::QueryServiceClient *query = new Nepomuk::Search::QueryServiceClient( this );

  connect( query, SIGNAL( newEntries( const QList<Nepomuk::Search::Result>& ) ),
           this, SLOT( hitsAdded( const QList<Nepomuk::Search::Result>& ) ) );
  connect( query, SIGNAL( entriesRemoved( const QList<QUrl>& ) ),
           this, SLOT( hitsRemoved( const QList<QUrl>& ) ) );

  mMutex.lock();
  mQueryMap.insert( query, collection.id() );
  mQueryInvMap.insert( collection.id(), query ); // needed for fast lookup in removeSearch()
  mMutex.unlock();

  // query with SPARQL statement
  query->query( collection.queryString() );
}

void NepomukSearchEngine::removeSearch( qint64 collectionId )
{
  Nepomuk::Search::QueryServiceClient *query = mQueryInvMap.value( collectionId );
  if ( !query ) {
    qWarning() << "Nepomuk QueryServer: Query could not be removed!";
    return;
  }

  query->close();

  // cleanup mappings
  mMutex.lock();
  mQueryInvMap.remove( collectionId );
  mQueryMap.remove( query );
  mMutex.unlock();

  query->deleteLater();
}

void NepomukSearchEngine::reloadSearches()
{
  SelectQueryBuilder<Collection> qb;
  qb.addValueCondition( Collection::queryLanguageFullColumnName(), Query::Equals, QLatin1String( "SPARQL" ) );
  if ( !qb.exec() ) {
    qWarning() << "Nepomuk QueryServer: Unable to execute query!";
    return;
  }

  Q_FOREACH ( const Collection &collection, qb.result() ) {
    qDebug() << "adding search" << collection.name();
    addSearch( collection );
  }
}

void NepomukSearchEngine::stopSearches()
{
  SelectQueryBuilder<Collection> qb;
  qb.addValueCondition( Collection::queryLanguageFullColumnName(), Query::Equals, QLatin1String( "SPARQL" ) );
  if ( !qb.exec() ) {
    qWarning() << "Nepomuk QueryServer: Unable to execute query!";
    return;
  }

  Q_FOREACH ( const Collection &collection, qb.result() ) {
    qDebug() << "removing search" << collection.name();
    removeSearch( collection.id() );
  }
}

void NepomukSearchEngine::hitsAdded( const QList<Nepomuk::Search::Result>& entries )
{
  Nepomuk::Search::QueryServiceClient *query = qobject_cast<Nepomuk::Search::QueryServiceClient*>( sender() );
  if ( !query ) {
    qWarning() << "Nepomuk QueryServer: Got signal from non-existing search query!";
    return;
  }

  mMutex.lock();
  qint64 collectionId = mQueryMap.value( query );
  mMutex.unlock();
  const Collection collection = Collection::retrieveById( collectionId );

  Q_FOREACH( const Nepomuk::Search::Result &result, entries ) {
    const qint64 itemId = uriToItemId( result.resourceUri() );

    if ( itemId == -1 )
      continue;

    Entity::addToRelation<CollectionPimItemRelation>( collectionId, itemId );
    mCollector->itemLinked( PimItem::retrieveById( itemId ), collection );
  }

  mCollector->dispatchNotifications();
}

void NepomukSearchEngine::hitsRemoved( const QList<QUrl> &entries )
{
  Nepomuk::Search::QueryServiceClient *query = qobject_cast<Nepomuk::Search::QueryServiceClient*>( sender() );
  if ( !query ) {
    qWarning() << "Nepomuk QueryServer: Got signal from non-existing search query!";
    return;
  }

  mMutex.lock();
  qint64 collectionId = mQueryMap.value( query );
  mMutex.unlock();
  const Collection collection = Collection::retrieveById( collectionId );

  Q_FOREACH( const QUrl &uri, entries ) {
    const qint64 itemId = uriToItemId( uri );

    if ( itemId == -1 )
      continue;

    Entity::removeFromRelation<CollectionPimItemRelation>( collectionId, itemId );
    mCollector->itemUnlinked( PimItem::retrieveById( itemId ), collection );
  }

  mCollector->dispatchNotifications();
}

#include "nepomuksearchengine.moc"
