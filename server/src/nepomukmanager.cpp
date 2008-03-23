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

#include "nepomukmanager.h"

#include "searchqueryiteratorinterface.h"

#include "entities.h"

using namespace Akonadi;

static qint64 uriToItemId( const QString &uri )
{
  if ( uri.isEmpty() )
    return -1;

  bool ok = false;

  const QUrl url( uri );
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

  mSearchInterface = new org::kde::Akonadi::Search( QLatin1String( "org.kde.Akonadi.Search" ), QLatin1String( "/Search" ),
                                                    QDBusConnection::sessionBus(), this );

  if ( !mSearchInterface->isValid() ) {
    qWarning() << "Nepomuk QueryServer interface not found!";
    mValid = false;
  }
}

NepomukManager::~NepomukManager()
{
  stopSearches();
}

bool NepomukManager::addSearch( const Location &location )
{
  if ( !mSearchInterface->isValid() || !mValid )
    return false;

  QMutexLocker lock( &mMutex );
  if ( location.remoteId().isEmpty() )
    return false;

  // TODO: search statement is stored in remoteId currently, port to attributes!
  const QString searchStatement = location.remoteId();

  const QString queryPath = mSearchInterface->createQuery( searchStatement );
  org::kde::Akonadi::SearchQuery *query = new org::kde::Akonadi::SearchQuery( QLatin1String( "org.kde.Akonadi.Search" ),
                                                                              queryPath, QDBusConnection::sessionBus(), 0 );

  if ( !query->isValid() ) {
    qWarning() << "Nepomuk QueryServer: Query could not be instantiated!";
    return false;
  }

  connect( query, SIGNAL( hitsChanged( const QStringList& ) ),
           this, SLOT( hitsAdded( const QStringList& ) ) );
  connect( query, SIGNAL( hitsRemoved( const QStringList& ) ),
           this, SLOT( hitsRemoved( const QStringList& ) ) );

  mQueryMap.insert( query, location.id() );
  mQueryInvMap.insert( location.id(), query ); // needed for fast lookup in removeSearch()

  // load all hits that are currently available...
  const QString iteratorPath = query->allHits();

  org::kde::Akonadi::SearchQueryIterator *iterator =
                  new org::kde::Akonadi::SearchQueryIterator( QLatin1String( "org.kde.Akonadi.Search" ), iteratorPath,
                                                              QDBusConnection::sessionBus(), 0 );

  while ( iterator->next() ) {
    const QString uri = iterator->currentUri();

    const qint64 itemId = uriToItemId( uri );
    if ( itemId == -1 ) {
      qWarning() << "Nepomuk QueryServer: Retrieved invalid item id from server!";
      continue;
    }

    Entity::addToRelation<LocationPimItemRelation>( location.id(), itemId );
  }

  iterator->close();

  // ... and start the change monitoring for future changes
  query->start();

  qDebug( "--------------- added search for loc %lld", location.id() );

  return true;
}

bool NepomukManager::removeSearch( qint64 locationId )
{
  org::kde::Akonadi::SearchQuery *query = mQueryInvMap.value( locationId );
  if ( !query || !query->isValid() ) {
    qWarning() << "Nepomuk QueryServer: Query could not be removed!";
  } else {
    query->stop();
    query->close();
  }

  // cleanup mappings
  mQueryInvMap.remove( locationId );
  mQueryMap.remove( query );

  qDebug( "--------------- removed search for loc %lld", locationId );

  return true;
}

void NepomukManager::reloadSearches()
{
  Resource resource = Resource::retrieveByName( QLatin1String( "akonadi_search_resource" ) );
  if ( !resource.isValid() ) {
    qWarning() << "Nepomuk QueryServer: No valid search resource found!";
    return;
  }

  const Location::List locations = resource.locations();
  Q_FOREACH ( const Location location, locations ) {
    addSearch( location );
  }
}

void NepomukManager::stopSearches()
{
  Resource resource = Resource::retrieveByName( QLatin1String( "akonadi_search_resource" ) );
  if ( !resource.isValid() ) {
    qWarning() << "Nepomuk QueryServer: No valid search resource found!";
    return;
  }

  const Location::List locations = resource.locations();
  Q_FOREACH ( const Location location, locations ) {
    removeSearch( location.id() );
  }
}

void NepomukManager::hitsAdded( const QStringList &hits )
{
  qDebug( "--------------- hits added 1" );

  org::kde::Akonadi::SearchQuery *query = qobject_cast<org::kde::Akonadi::SearchQuery*>( sender() );
  if ( !query ) {
    qWarning() << "Nepomuk QueryServer: Got signal from non-existing search query!";
    return;
  }

  qDebug( "--------------- hits added 2" );

  mMutex.lock();
  qint64 locationId = mQueryMap.value( query );
  mMutex.unlock();

  Q_FOREACH( const QString uri, hits ) {
    const qint64 itemId = uriToItemId( uri );

    if ( itemId == -1 ) {
      qWarning() << "Nepomuk QueryServer: Retrieved invalid item id from server!";
      continue;
    }

    Entity::addToRelation<LocationPimItemRelation>( locationId, itemId );
  }
}

void NepomukManager::hitsRemoved( const QStringList &hits )
{
  qDebug( "--------------- hits removed 1" );

  org::kde::Akonadi::SearchQuery *query = qobject_cast<org::kde::Akonadi::SearchQuery*>( sender() );
  if ( !query ) {
    qWarning() << "Nepomuk QueryServer: Got signal from non-existing search query!";
    return;
  }

  qDebug( "--------------- hits removed 2" );

  mMutex.lock();
  qint64 locationId = mQueryMap.value( query );
  mMutex.unlock();

  Q_FOREACH( const QString uri, hits ) {
    const qint64 itemId = uriToItemId( uri );

    if ( itemId == -1 ) {
      qWarning() << "Nepomuk QueryServer: Retrieved invalid item id from server!";
      continue;
    }

    Entity::removeFromRelation<LocationPimItemRelation>( locationId, itemId );
  }
}

#include "nepomukmanager.moc"
