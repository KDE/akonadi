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

#include <QtCore/QDebug>
#include <QDBusConnection>

#include "notificationmanager.h"
#include "notificationmanageradaptor.h"
#include "tracer.h"
#include "storage/datastore.h"

using namespace Akonadi;

NotificationManager* NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  NotificationMessage::registerDBusTypes();

  new NotificationManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String("/notifications"),
    this, QDBusConnection::ExportAdaptors );
}

NotificationManager::~NotificationManager()
{
}

NotificationManager* NotificationManager::self()
{
  if ( !mSelf )
    mSelf = new NotificationManager();

  return mSelf;
}

void NotificationManager::connectDatastore( DataStore * store )
{
  connect( store->notificationCollector(), SIGNAL(itemAddedNotification(QByteArray,int,QString,int,QString,QByteArray)),
           SLOT(slotItemAdded(QByteArray,int,QString,int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(itemChangedNotification(QByteArray,int,QString,int,QString,QByteArray)),
           SLOT(slotItemChanged(QByteArray,int,QString,int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(itemRemovedNotification(QByteArray,int,QString,int,QString,QByteArray)),
           SLOT(slotItemRemoved(QByteArray,int,QString,int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionAddedNotification(QByteArray,int,QString,QByteArray)),
           SLOT(slotCollectionAdded(QByteArray,int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionChangedNotification(QByteArray,int,QString,QByteArray)),
           SLOT(slotCollectionChanged(QByteArray,int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionRemovedNotification(QByteArray,int,QString,QByteArray)),
           SLOT(slotCollectionRemoved(QByteArray,int,QString,QByteArray)) );
}


void Akonadi::NotificationManager::slotItemAdded( const QByteArray &sessionId, int uid, const QString &remoteId,
                                                  int collection, const QString &mimeType,
                                                  const QByteArray &resource )
{
  NotificationMessage m;
  m.setType( NotificationMessage::Item );
  m.setOperation( NotificationMessage::Add );
  m.setSessionId( sessionId );
  m.setUid( uid );
  m.setRemoteId( remoteId );
  m.setMimeType( mimeType );
  m.setResource( resource );
  m.setParentCollection( collection );

  Tracer::self()->signal( "NotificationManager::notify", m.toString() );
  emit notify( m );
}

void Akonadi::NotificationManager::slotItemChanged( const QByteArray &sessionId, int uid, const QString &remoteId,
                                                    int collection, const QString &mimetype,
                                                    const QByteArray &resource )
{
  NotificationMessage m;
  m.setType( NotificationMessage::Item );
  m.setOperation( NotificationMessage::Modify );
  m.setSessionId( sessionId );
  m.setUid( uid );
  m.setRemoteId( remoteId );
  m.setMimeType( mimetype );
  m.setResource( resource );
  m.setParentCollection( collection );

  Tracer::self()->signal( "NotificationManager::notify", m.toString() );
  emit notify( m );
}

void Akonadi::NotificationManager::slotItemRemoved( const QByteArray &sessionId, int uid, const QString &remoteId,
                                                    int collection, const QString &mimetype,
                                                    const QByteArray &resource )
{
  NotificationMessage m;
  m.setType( NotificationMessage::Item );
  m.setOperation( NotificationMessage::Remove );
  m.setSessionId( sessionId );
  m.setUid( uid );
  m.setRemoteId( remoteId );
  m.setMimeType( mimetype );
  m.setResource( resource );
  m.setParentCollection( collection );

  Tracer::self()->signal( "NotificationManager::notify", m.toString() );
  emit notify( m );
}

void Akonadi::NotificationManager::slotCollectionAdded(const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray &resource)
{
  NotificationMessage m;
  m.setType( NotificationMessage::Collection );
  m.setOperation( NotificationMessage::Add );
  m.setSessionId( sessionId );
  m.setUid( collection );
  m.setRemoteId( remoteId );
  m.setResource( resource );

  Tracer::self()->signal( "NotificationManager::notify", m.toString() );
  emit notify( m );
}

void Akonadi::NotificationManager::slotCollectionChanged(const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray & resource)
{
  NotificationMessage m;
  m.setType( NotificationMessage::Collection );
  m.setOperation( NotificationMessage::Modify );
  m.setSessionId( sessionId );
  m.setUid( collection );
  m.setRemoteId( remoteId );
  m.setResource( resource );

  Tracer::self()->signal( "NotificationManager::notify", m.toString() );
  emit notify( m );
}

void Akonadi::NotificationManager::slotCollectionRemoved(const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray &resource)
{
  NotificationMessage m;
  m.setType( NotificationMessage::Collection );
  m.setOperation( NotificationMessage::Remove );
  m.setSessionId( sessionId );
  m.setUid( collection );
  m.setRemoteId( remoteId );
  m.setResource( resource );

  Tracer::self()->signal( "NotificationManager::notify", m.toString() );
  emit notify( m );
}

#include "notificationmanager.moc"
