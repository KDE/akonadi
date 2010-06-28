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
#include "handlerhelper.h"

#include <QtCore/QDebug>

using namespace Akonadi;


NotificationCollector::NotificationCollector(QObject* parent) :
  QObject( parent ),
  mDb( 0 )
{
}

Akonadi::NotificationCollector::NotificationCollector(DataStore * db) :
  QObject( db ),
  mDb( db )
{
  connect( db, SIGNAL(transactionCommitted()), SLOT(transactionCommitted()) );
  connect( db, SIGNAL(transactionRolledBack()), SLOT(transactionRolledBack()) );
}

Akonadi::NotificationCollector::~NotificationCollector()
{
}

void Akonadi::NotificationCollector::itemAdded( const PimItem &item,
                                                const Collection &collection,
                                                const QString & mimeType,
                                                const QByteArray & resource )
{
  itemNotification( NotificationMessage::Add, item, collection, Collection(), mimeType, resource );
}

void Akonadi::NotificationCollector::itemChanged( const PimItem &item,
                                                  const QSet<QByteArray> &changedParts,
                                                  const Collection &collection,
                                                  const QString & mimeType,
                                                  const QByteArray & resource )
{
  itemNotification( NotificationMessage::Modify, item, collection, Collection(), mimeType, resource, changedParts );
}

void Akonadi::NotificationCollector::itemMoved( const PimItem &item,
                                                const Collection &collectionSrc,
                                                const Collection &collectionDest,
                                                const QString &mimeType,
                                                const QByteArray &sourceResource )
{
  itemNotification( NotificationMessage::Move, item, collectionSrc, collectionDest, mimeType, sourceResource );
}


void Akonadi::NotificationCollector::itemRemoved( const PimItem &item,
                                                  const Collection &collection,
                                                  const QString & mimeType,
                                                  const QByteArray & resource )
{
  itemNotification( NotificationMessage::Remove, item, collection, Collection(), mimeType, resource );
}

void NotificationCollector::itemLinked(const PimItem & item, const Collection & collection)
{
  itemNotification( NotificationMessage::Link, item, collection, Collection(), QString(), QByteArray() );
}

void NotificationCollector::itemUnlinked(const PimItem & item, const Collection & collection)
{
  itemNotification( NotificationMessage::Unlink, item, collection, Collection(), QString(), QByteArray() );
}

void Akonadi::NotificationCollector::collectionAdded( const Collection &collection,
                                                      const QByteArray &resource )
{
  collectionNotification( NotificationMessage::Add, collection, collection.parentId(), -1, resource );
}

void Akonadi::NotificationCollector::collectionChanged( const Collection &collection,
                                                        const QList<QByteArray> &changes,
                                                        const QByteArray &resource )
{
  collectionNotification( NotificationMessage::Modify, collection, collection.parentId(), -1, resource, changes.toSet() );
}

void NotificationCollector::collectionMoved( const Collection &collection,
                                             const Collection &source,
                                             const QByteArray &resource )
{
  collectionNotification( NotificationMessage::Move, collection, source.id(), collection.parentId(), resource, QSet<QByteArray>() << "PARENT" );
}

void Akonadi::NotificationCollector::collectionRemoved( const Collection &collection,
                                                        const QByteArray &resource )
{
  collectionNotification( NotificationMessage::Remove, collection, collection.parentId(), -1, resource );
}

void Akonadi::NotificationCollector::transactionCommitted()
{
  dispatchNotifications();
}

void Akonadi::NotificationCollector::transactionRolledBack()
{
  clear();
}

void Akonadi::NotificationCollector::clear()
{
  mNotifications.clear();
}

void NotificationCollector::setSessionId(const QByteArray &sessionId)
{
  mSessionId = sessionId;
}

void NotificationCollector::itemNotification( NotificationMessage::Operation op,
                                              const PimItem & item,
                                              const Collection & collection,
                                              const Collection & collectionDest,
                                              const QString & mimeType,
                                              const QByteArray & resource,
                                              const QSet<QByteArray> &parts )
{
  NotificationMessage msg;
  msg.setSessionId( mSessionId );
  msg.setType( NotificationMessage::Item );
  msg.setOperation( op );
  msg.setUid( item.id() );
  msg.setRemoteId( item.remoteId() );
  msg.setItemParts( parts );

  //HACK: store remoteRevision in itemparts for deletion
  if ( op == NotificationMessage::Remove )
    msg.setItemParts( QSet<QByteArray>() << item.remoteRevision().toUtf8() );

  // another HACK: store the destination resource for moves
  if ( op == NotificationMessage::Move )
    msg.setItemParts( QSet<QByteArray>() << collectionDest.resource().name().toLatin1() );

  Collection col = collection;
  if ( !col.isValid() )
    col = item.collection();
  msg.setParentCollection( col.id() );
  // will be valid if it is a move message
  msg.setParentDestCollection( collectionDest.id() );
  QString mt = mimeType;
  if ( mt.isEmpty() )
    mt = item.mimeType().name();
  msg.setMimeType( mt );
  QByteArray res = resource;
  if ( res.isEmpty() )
    res = col.resource().name().toLatin1();
  msg.setResource( res );
  dispatchNotification( msg );
}

void NotificationCollector::collectionNotification( NotificationMessage::Operation op,
                                                    const Collection & collection,
                                                    Collection::Id source,
                                                    Collection::Id destination,
                                                    const QByteArray & resource,
                                                    const QSet<QByteArray> &changes )
{
  NotificationMessage msg;
  msg.setType( NotificationMessage::Collection );
  msg.setOperation( op );
  msg.setSessionId( mSessionId );
  msg.setUid( collection.id() );
  msg.setRemoteId( collection.remoteId() );
  msg.setParentCollection( source );
  msg.setParentDestCollection( destination );
  msg.setItemParts( changes );

  //HACK: store remoteRevision in itemparts for deletion
  if ( op == NotificationMessage::Remove )
    msg.setItemParts( QSet<QByteArray>() << collection.remoteRevision().toUtf8() );

  QByteArray res = resource;
  if ( res.isEmpty() )
    res = collection.resource().name().toLatin1();
  msg.setResource( res );
  dispatchNotification( msg );
}

void NotificationCollector::dispatchNotification(const NotificationMessage & msg)
{
  if ( !mDb || mDb->inTransaction() ) {
    NotificationMessage::appendAndCompress( mNotifications, msg );
  } else {
    NotificationMessage::List l;
    l << msg;
    emit notify( l );
  }
}

void NotificationCollector::dispatchNotifications()
{
  emit notify( mNotifications );
  clear();
}

#include "notificationcollector.moc"
