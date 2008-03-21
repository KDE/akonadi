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

#include <akonadi/private/notificationmessage.h>

#include <QtCore/QDebug>

using namespace Akonadi;

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
                                                const Location &collection,
                                                const QString & mimeType,
                                                const QByteArray & resource )
{
  itemNotification( NotificationMessage::Add, item, collection, Location(), mimeType, resource );
}

void Akonadi::NotificationCollector::itemChanged( const PimItem &item,
                                                  const Location &collection,
                                                  const QString & mimeType,
                                                  const QByteArray & resource )
{
  itemNotification( NotificationMessage::Modify, item, collection, Location(), mimeType, resource );
}

void Akonadi::NotificationCollector::itemMoved( const PimItem &item,
                                                const Location &collectionSrc,
                                                const Location &collectionDest,
                                                const QString &mimeType,
                                                const QByteArray &resource )
{
  itemNotification( NotificationMessage::Move, item, collectionSrc, collectionDest, mimeType, resource );
}


void Akonadi::NotificationCollector::itemRemoved( const PimItem &item,
                                                  const Location &collection,
                                                  const QString & mimeType,
                                                  const QByteArray & resource )
{
  itemNotification( NotificationMessage::Remove, item, collection, Location(), mimeType, resource );
}

void Akonadi::NotificationCollector::collectionAdded( const Location &collection,
                                                      const QByteArray &resource )
{
  collectionNotification( NotificationMessage::Add, collection, resource );
}

void Akonadi::NotificationCollector::collectionChanged( const Location &collection,
                                                        const QByteArray &resource )
{
  collectionNotification( NotificationMessage::Modify, collection, resource );
}

void Akonadi::NotificationCollector::collectionRemoved( const Location &collection,
                                                        const QByteArray &resource )
{
  collectionNotification( NotificationMessage::Remove, collection, resource );
}

void Akonadi::NotificationCollector::transactionCommitted()
{
  emit notify( mNotifications );
  clear();
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
                                              const Location & collection,
                                              const Location & collectionDest,
                                              const QString & mimeType,
                                              const QByteArray & resource)
{
  NotificationMessage msg;
  msg.setSessionId( mSessionId );
  msg.setType( NotificationMessage::Item );
  msg.setOperation( op );
  msg.setUid( item.id() );
  msg.setRemoteId( QString::fromUtf8( item.remoteId() ) );

  Location loc = collection;
  if ( !loc.isValid() )
    loc = item.location();
  msg.setParentCollection( loc.id() );
  // will be valid if it is a move message
  msg.setParentDestCollection( collectionDest.id() );
  QString mt = mimeType;
  if ( mt.isEmpty() )
    mt = item.mimeType().name();
  msg.setMimeType( mt );
  QByteArray res = resource;
  if ( res.isEmpty() )
    res = loc.resource().name().toLatin1();
  msg.setResource( res );
  dispatchNotification( msg );
}

void NotificationCollector::collectionNotification( NotificationMessage::Operation op,
                                                    const Location & collection,
                                                    const QByteArray & resource)
{
  NotificationMessage msg;
  msg.setType( NotificationMessage::Collection );
  msg.setOperation( op );
  msg.setSessionId( mSessionId );
  msg.setUid( collection.id() );
  msg.setRemoteId( collection.remoteId() );
  msg.setParentCollection( collection.parentId() );

  QByteArray res = resource;
  if ( res.isEmpty() )
    res = collection.resource().name().toLatin1();
  msg.setResource( res );
  dispatchNotification( msg );
}

void NotificationCollector::dispatchNotification(const NotificationMessage & msg)
{
  if ( mDb->inTransaction() ) {
    NotificationMessage::appendAndCompress( mNotifications, msg );
  } else {
    NotificationMessage::List l;
    l << msg;
    emit notify( l );
  }
}

#include "notificationcollector.moc"
