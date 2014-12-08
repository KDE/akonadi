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

#include "notificationcollector.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/collectionstatistics.h"
#include "handlerhelper.h"
#include "cachecleaner.h"
#include "intervalcheck.h"
#include "search/searchmanager.h"
#include "akonadi.h"
#include <search.h>

#include <QtCore/QDebug>

using namespace Akonadi;
using namespace Akonadi::Server;

NotificationCollector::NotificationCollector( QObject *parent )
  : QObject( parent )
  , mDb( 0 )
{
}

NotificationCollector::NotificationCollector( DataStore *db )
  : QObject( db )
  , mDb( db )
{
  connect( db, SIGNAL(transactionCommitted()), SLOT(transactionCommitted()) );
  connect( db, SIGNAL(transactionRolledBack()), SLOT(transactionRolledBack()) );
}

NotificationCollector::~NotificationCollector()
{
}

void NotificationCollector::itemAdded( const PimItem &item,
                                       const Collection &collection,
                                       const QByteArray &resource )
{
  SearchManager::instance()->scheduleSearchUpdate();
  itemNotification( NotificationMessageV2::Add, item, collection, Collection(), resource );
}

void NotificationCollector::itemChanged( const PimItem &item,
                                         const QSet<QByteArray> &changedParts,
                                         const Collection &collection,
                                         const QByteArray &resource )
{
  SearchManager::instance()->scheduleSearchUpdate();
  itemNotification( NotificationMessageV2::Modify, item, collection, Collection(), resource, changedParts );
}

void NotificationCollector::itemsFlagsChanged( const PimItem::List &items,
                                               const QSet< QByteArray > &addedFlags,
                                               const QSet< QByteArray > &removedFlags,
                                               const Collection &collection,
                                               const QByteArray &resource )
{
  itemNotification( NotificationMessageV2::ModifyFlags, items, collection, Collection(), resource, QSet<QByteArray>(), addedFlags, removedFlags );
}

void NotificationCollector::itemsTagsChanged(const PimItem::List& items,
                                             const QSet<qint64>& addedTags,
                                             const QSet<qint64>& removedTags,
                                             const Collection& collection,
                                             const QByteArray& resource)
{
  itemNotification( NotificationMessageV2::ModifyTags, items, collection, Collection(), resource, QSet<QByteArray>(), QSet<QByteArray>(), QSet<QByteArray>(), addedTags, removedTags );
}

void NotificationCollector::itemsMoved( const PimItem::List &items,
                                        const Collection &collectionSrc,
                                        const Collection &collectionDest,
                                        const QByteArray &sourceResource )
{
  SearchManager::instance()->scheduleSearchUpdate();
  itemNotification( NotificationMessageV2::Move, items, collectionSrc, collectionDest, sourceResource );
}

void NotificationCollector::itemsRemoved( const PimItem::List &items,
                                          const Collection &collection,
                                          const QByteArray &resource )
{
  itemNotification( NotificationMessageV2::Remove, items, collection, Collection(), resource );
}

void NotificationCollector::itemsLinked( const PimItem::List &items, const Collection &collection )
{
  itemNotification( NotificationMessageV2::Link, items, collection, Collection(), QByteArray() );
}

void NotificationCollector::itemsUnlinked( const PimItem::List &items, const Collection &collection )
{
  itemNotification( NotificationMessageV2::Unlink, items, collection, Collection(), QByteArray() );
}

void NotificationCollector::collectionAdded( const Collection &collection,
                                             const QByteArray &resource )
{
  if ( AkonadiServer::instance()->cacheCleaner() ) {
    AkonadiServer::instance()->cacheCleaner()->collectionAdded( collection.id() );
  }
  if ( AkonadiServer::instance()->intervalChecker() ) {
    AkonadiServer::instance()->intervalChecker()->collectionAdded( collection.id() );
  }
  collectionNotification( NotificationMessageV2::Add, collection, collection.parentId(), -1, resource );
}

void NotificationCollector::collectionChanged( const Collection &collection,
                                               const QList<QByteArray> &changes,
                                               const QByteArray &resource )
{
  if ( AkonadiServer::instance()->cacheCleaner() ) {
    AkonadiServer::instance()->cacheCleaner()->collectionAdded( collection.id() );
  }
  if ( AkonadiServer::instance()->intervalChecker() ) {
    AkonadiServer::instance()->intervalChecker()->collectionAdded( collection.id() );
  }
  CollectionStatistics::self()->invalidateCollection(collection);
  collectionNotification( NotificationMessageV2::Modify, collection, collection.parentId(), -1, resource, changes.toSet() );
}

void NotificationCollector::collectionMoved( const Collection &collection,
                                             const Collection &source,
                                             const QByteArray &resource,
                                             const QByteArray &destResource )
{
  if ( AkonadiServer::instance()->cacheCleaner() ) {
    AkonadiServer::instance()->cacheCleaner()->collectionChanged( collection.id() );
  }
  if ( AkonadiServer::instance()->intervalChecker() ) {
    AkonadiServer::instance()->intervalChecker()->collectionChanged( collection.id() );
  }
  collectionNotification( NotificationMessageV2::Move, collection, source.id(), collection.parentId(), resource, QSet<QByteArray>(), destResource );
}

void NotificationCollector::collectionRemoved( const Collection &collection,
                                               const QByteArray &resource )
{
  if ( AkonadiServer::instance()->cacheCleaner() ) {
    AkonadiServer::instance()->cacheCleaner()->collectionRemoved( collection.id() );
  }
  if ( AkonadiServer::instance()->intervalChecker() ) {
    AkonadiServer::instance()->intervalChecker()->collectionRemoved( collection.id() );
  }
  CollectionStatistics::self()->invalidateCollection(collection);

  collectionNotification( NotificationMessageV2::Remove, collection, collection.parentId(), -1, resource );
}

void NotificationCollector::collectionSubscribed( const Collection &collection,
                                                  const QByteArray &resource )
{
  if ( AkonadiServer::instance()->cacheCleaner() ) {
    AkonadiServer::instance()->cacheCleaner()->collectionAdded( collection.id() );
  }
  if ( AkonadiServer::instance()->intervalChecker() ) {
    AkonadiServer::instance()->intervalChecker()->collectionAdded( collection.id() );
  }
  collectionNotification( NotificationMessageV2::Subscribe, collection, collection.parentId(), -1, resource, QSet<QByteArray>() );
}

void NotificationCollector::collectionUnsubscribed( const Collection &collection,
                                                    const QByteArray &resource )
{
  if ( AkonadiServer::instance()->cacheCleaner() ) {
    AkonadiServer::instance()->cacheCleaner()->collectionRemoved( collection.id() );
  }
  if ( AkonadiServer::instance()->intervalChecker() ) {
    AkonadiServer::instance()->intervalChecker()->collectionRemoved( collection.id() );
  }
  CollectionStatistics::self()->invalidateCollection(collection);

  collectionNotification( NotificationMessageV2::Unsubscribe, collection, collection.parentId(), -1, resource, QSet<QByteArray>() );
}

void NotificationCollector::tagAdded(const Tag &tag )
{
  tagNotification( NotificationMessageV2::Add, tag );
}

void NotificationCollector::tagChanged( const Tag &tag )
{
  tagNotification( NotificationMessageV2::Modify, tag );
}

void NotificationCollector::tagRemoved( const Tag &tag )
{
  tagNotification( NotificationMessageV2::Remove, tag );
}

void NotificationCollector::transactionCommitted()
{
  dispatchNotifications();
}

void NotificationCollector::transactionRolledBack()
{
  clear();
}

void NotificationCollector::clear()
{
  mNotifications.clear();
}

void NotificationCollector::setSessionId( const QByteArray &sessionId )
{
  mSessionId = sessionId;
}

void NotificationCollector::itemNotification( NotificationMessageV2::Operation op,
                                              const PimItem &item,
                                              const Collection &collection,
                                              const Collection &collectionDest,
                                              const QByteArray &resource,
                                              const QSet<QByteArray> &parts )
{
  PimItem::List items;
  items << item;
  itemNotification( op, items, collection, collectionDest, resource, parts );
}

void NotificationCollector::itemNotification( NotificationMessageV2::Operation op,
                                              const PimItem::List &items,
                                              const Collection &collection,
                                              const Collection &collectionDest,
                                              const QByteArray &resource,
                                              const QSet<QByteArray> &parts,
                                              const QSet<QByteArray> &addedFlags,
                                              const QSet<QByteArray> &removedFlags,
                                              const QSet<qint64> &addedTags,
                                              const QSet<qint64> &removedTags )
{
  Collection notificationDestCollection;
  QMap<Entity::Id, QList<PimItem> > vCollections;

  if ( ( op == NotificationMessageV2::Modify ) ||
       ( op == NotificationMessageV2::ModifyFlags ) ||
       ( op == NotificationMessageV2::ModifyTags ) ) {
    vCollections = DataStore::self()->virtualCollections( items );
  }

  NotificationMessageV3 msg;
  msg.setSessionId( mSessionId );
  msg.setType( NotificationMessageV2::Items );
  msg.setOperation( op );

  msg.setItemParts( parts );
  msg.setAddedFlags( addedFlags );
  msg.setRemovedFlags( removedFlags );
  msg.setAddedTags( addedTags );
  msg.setRemovedTags( removedTags );

  if ( collectionDest.isValid() ) {
    QByteArray destResourceName;
    destResourceName = collectionDest.resource().name().toLatin1();
    msg.setDestinationResource( destResourceName );
  }

  msg.setParentDestCollection( collectionDest.id() );

  /* Notify all virtual collections the items are linked to. */
  for (auto iter = vCollections.constBegin(); iter != vCollections.constEnd(); ++iter) {
    NotificationMessageV3 copy = msg;

    Q_FOREACH ( const PimItem &item, iter.value() ) {
      copy.addEntity( item.id(), item.remoteId(), item.remoteRevision(), item.mimeType().name() );
    }
    copy.setParentCollection( iter.key() );
    copy.setResource( resource );

    CollectionStatistics::self()->invalidateCollection(Collection::retrieveById(iter.key()));
    dispatchNotification( copy );
  }

  Q_FOREACH ( const PimItem &item, items ) {
    msg.addEntity( item.id(), item.remoteId(), item.remoteRevision(), item.mimeType().name() );
  }

  Collection col;
  if ( !collection.isValid() ) {
    msg.setParentCollection( items.first().collection().id() );
    col = items.first().collection();
  } else {
    msg.setParentCollection( collection.id() );
    col = collection;
  }

  QByteArray res = resource;
  if ( res.isEmpty() ) {
    res = col.resource().name().toLatin1();
  }
  msg.setResource( res );

  CollectionStatistics::self()->invalidateCollection(col);
  dispatchNotification( msg );
}

void NotificationCollector::collectionNotification( NotificationMessageV2::Operation op,
                                                    const Collection &collection,
                                                    Collection::Id source,
                                                    Collection::Id destination,
                                                    const QByteArray &resource,
                                                    const QSet<QByteArray> &changes,
                                                    const QByteArray &destResource )
{
  NotificationMessageV3 msg;
  msg.setType( NotificationMessageV2::Collections );
  msg.setOperation( op );
  msg.setSessionId( mSessionId );
  msg.addEntity( collection.id(), collection.remoteId(), collection.remoteRevision() );
  msg.setParentCollection( source );
  msg.setParentDestCollection( destination );
  msg.setDestinationResource( destResource );
  msg.setItemParts( changes );

  QByteArray res = resource;
  if ( res.isEmpty() ) {
    res = collection.resource().name().toLatin1();
  }
  msg.setResource( res );

  dispatchNotification( msg );
}

void NotificationCollector::tagNotification( NotificationMessageV2::Operation op,
                                             const Tag &tag )
{
  NotificationMessageV3 msg;
  msg.setType( NotificationMessageV2::Tags );
  msg.setOperation( op );
  msg.setSessionId( mSessionId );
  msg.addEntity( tag.id() );

  dispatchNotification( msg );
}


void NotificationCollector::dispatchNotification( const NotificationMessageV3 &msg )
{
  if ( !mDb || mDb->inTransaction() ) {
    NotificationMessageV3::appendAndCompress( mNotifications, msg );
  } else {
    NotificationMessageV3::List l;
    l << msg;
    Q_EMIT notify( l );
  }
}

void NotificationCollector::dispatchNotifications()
{
  if ( !mNotifications.isEmpty() ) {
    Q_EMIT notify( mNotifications );
    clear();
  }
}
