/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "xesamsearchengine.h"

#include "entities.h"
#include "notificationmanager.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "xesaminterface.h"
#include "xesamtypes.h"

#include <QtCore/QDebug>

using namespace Akonadi;

static qint64 uriToItemId( const QString &urlString )
{
  const QUrl url( urlString );
  bool ok = false;

  const qint64 id = url.queryItemValue( QLatin1String( "item" ) ).toLongLong( &ok );

  if ( !ok )
    return -1;
  else
    return id;
}

XesamSearchEngine::XesamSearchEngine( QObject *parent )
  : QObject( parent ),
    mValid( true ),
    mCollector( new NotificationCollector( this ) )
{
  NotificationManager::self()->connectNotificationCollector( mCollector );

  QDBusServiceWatcher *watcher =
    new QDBusServiceWatcher( QLatin1String( "org.freedesktop.xesam.searcher" ),
                             QDBusConnection::sessionBus(),
                             QDBusServiceWatcher::WatchForRegistration, this );
  connect( watcher, SIGNAL( serviceRegistered( QString ) ), SLOT( initializeSearchInterface() ) );

  initializeSearchInterface();

  if ( !mValid )
    qWarning() << "No valid XESAM interface found!";
}

XesamSearchEngine::~XesamSearchEngine()
{
  stopSearches();
  if ( !mSession.isEmpty() )
    mInterface->CloseSession( mSession );
}

void XesamSearchEngine::initializeSearchInterface()
{
  mInterface = new OrgFreedesktopXesamSearchInterface(
      QLatin1String( "org.freedesktop.xesam.searcher" ),
      QLatin1String( "/org/freedesktop/xesam/searcher/main" ),
      QDBusConnection::sessionBus(), this );

  if ( mInterface->isValid() ) {
    mSession = mInterface->NewSession();
    QDBusVariant result = mInterface->SetProperty( mSession, QLatin1String( "search.live" ), QDBusVariant( true ) );
    mValid = mValid && result.variant().toBool();

    connect( mInterface, SIGNAL( HitsAdded( QString, uint ) ), SLOT( slotHitsAdded( QString, uint ) ) );
    connect( mInterface, SIGNAL( HitsRemoved( QString, QList<uint> ) ), SLOT( slotHitsRemoved( QString, QList<uint> ) ) );
    connect( mInterface, SIGNAL( HitsModified( QString, QList<uint> ) ), SLOT( slotHitsModified( QString, QList<uint> ) ) );

    mValid = true;
    reloadSearches();
  } else {
    mValid = false;
  }
}

void XesamSearchEngine::slotHitsAdded( const QString &search, uint count )
{
  qDebug() << "hits added: " << search << count;
  mMutex.lock();
  const qint64 collectionId = mSearchMap.value( search );
  mMutex.unlock();

  if ( collectionId <= 0 || count <= 0 )
    return;

  const Collection collection = Collection::retrieveById( collectionId );

  qDebug() << "calling GetHits";

  const QVector<QList<QVariant> > results = mInterface->GetHits( search, count );
  qDebug() << "GetHits returned:" << results.count();

  typedef QList<QVariant> VariantList;
  foreach ( const VariantList &list, results ) {
    if ( list.isEmpty() )
      continue;

    const qint64 itemId = uriToItemId( list.first().toString() );
    if ( itemId == -1 )
      continue;

    Entity::addToRelation<CollectionPimItemRelation>( collectionId, itemId );
    mCollector->itemLinked( PimItem::retrieveById( itemId ), collection );
  }

  mCollector->dispatchNotifications();
}

void XesamSearchEngine::slotHitsRemoved( const QString &search, const QList<uint> &hits )
{
  qDebug() << "hits removed: " << search << hits;
  mMutex.lock();
  const qint64 collectionId = mSearchMap.value( search );
  mMutex.unlock();

  if ( collectionId <= 0 )
    return;

  const Collection collection = Collection::retrieveById( collectionId );

  const QVector<QList<QVariant> > results = mInterface->GetHitData( search, hits, QStringList( QLatin1String( "uri" ) ) );
  typedef QList<QVariant> VariantList;
  foreach ( const VariantList &list, results ) {
    if ( list.isEmpty() )
      continue;

    const qint64 itemId = uriToItemId( list.first().toString() );
    if ( itemId == -1 )
      continue;

    Entity::removeFromRelation<CollectionPimItemRelation>( collectionId, itemId );
    mCollector->itemUnlinked( PimItem::retrieveById( itemId ), collection );
  }

  mCollector->dispatchNotifications();
}

void XesamSearchEngine::slotHitsModified( const QString &search, const QList<uint> &hits )
{
  qDebug() << "hits modified: " << search << hits;
}

void XesamSearchEngine::reloadSearches()
{
  SelectQueryBuilder<Collection> qb;
  qb.addValueCondition( Collection::queryLanguageFullColumnName(), Query::Equals, QLatin1String( "XESAM" ) );
  if ( !qb.exec() ) {
    qWarning() << "Unable to execute query!";
    return;
  }

  Q_FOREACH ( const Collection &collection, qb.result() ) {
    mMutex.lock();
    if ( mInvSearchMap.contains( collection.id() ) ) {
      mMutex.unlock();
      qDebug() << "updating search" << collection.name();
      removeSearch( collection.id() );
    } else  {
      mMutex.unlock();
      qDebug() << "adding search" << collection.name();
    }
    addSearch( collection );
  }
}

void XesamSearchEngine::addSearch( const Collection &collection )
{
  if ( !mInterface->isValid() || !mValid || collection.queryLanguage() != QLatin1String( "XESAM" ) )
    return;

  if ( collection.remoteId().isEmpty() )
    return;

  const QString searchId = mInterface->NewSearch( mSession, collection.remoteId() );
  qDebug() << "XesamSearchEngine::addSeach" << collection.name() << searchId;

  mMutex.lock();
  mSearchMap.insert( searchId, collection.id() );
  mInvSearchMap.insert( collection.id(), searchId );
  mMutex.unlock();

  mInterface->StartSearch( searchId );
}

void XesamSearchEngine::removeSearch( qint64 collectionId )
{
  mMutex.lock();
  const QString searchId = mInvSearchMap.value( collectionId );
  mMutex.unlock();

  if ( searchId.isEmpty() )
    return;

  mInterface->CloseSearch( searchId );

  mMutex.lock();
  mInvSearchMap.remove( collectionId );
  mSearchMap.remove( searchId );
  mMutex.unlock();
}

void XesamSearchEngine::stopSearches()
{
  SelectQueryBuilder<Collection> qb;
  qb.addValueCondition( Collection::queryLanguageFullColumnName(), Query::Equals, QLatin1String( "XESAM" ) );
  if ( !qb.exec() ) {
    qWarning() << "Unable to execute query!";
    return;
  }

  Q_FOREACH ( const Collection &collection, qb.result() ) {
    qDebug() << "removing search" << collection.name();
    removeSearch( collection.id() );
  }
}

#include "xesamsearchengine.moc"
