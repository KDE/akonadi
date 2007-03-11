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
#include "tracer.h"
#include "storage/datastore.h"

using namespace Akonadi;

NotificationManager* NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  QDBusConnection::sessionBus().registerObject( QLatin1String("/notifications"),
    this, QDBusConnection::ExportScriptableSignals );
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
  connect( store->notificationCollector(), SIGNAL(itemAddedNotification(QByteArray,int,QString,int,QByteArray,QByteArray)),
           SLOT(slotItemAdded(QByteArray,int,QString,int,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(itemChangedNotification(QByteArray,int,QString,int,QByteArray,QByteArray)),
           SLOT(slotItemChanged(QByteArray,int,QString,int,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(itemRemovedNotification(QByteArray,int,QString,int,QByteArray,QByteArray)),
           SLOT(slotItemRemoved(QByteArray,int,QString,int,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionAddedNotification(QByteArray,int,QString,QByteArray)),
           SLOT(slotCollectionAdded(QByteArray,int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionChangedNotification(QByteArray,int,QString,QByteArray)),
           SLOT(slotCollectionChanged(QByteArray,int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionRemovedNotification(QByteArray,int,QString,QByteArray)),
           SLOT(slotCollectionRemoved(QByteArray,int,QString,QByteArray)) );
}


void Akonadi::NotificationManager::slotItemAdded( const QByteArray &sessionId, int uid, const QString &remoteId,
                                                  int collection, const QByteArray &mimeType,
                                                  const QByteArray &resource )
{
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4, Session: %5" )
      .arg( uid ).arg( collection ).arg( QString::fromLatin1(mimeType) ).arg( QString::fromLatin1(resource) ).arg( QString::fromLatin1(sessionId) );
  Tracer::self()->signal( "NotificationManager::itemAdded", msg );
  emit itemAdded( sessionId, uid, remoteId, collection, mimeType, resource );
}

void Akonadi::NotificationManager::slotItemChanged( const QByteArray &sessionId, int uid, const QString &remoteId,
                                                    int collection, const QByteArray &mimetype,
                                                    const QByteArray &resource )
{
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4, Session: %5" )
      .arg( uid ).arg( collection ).arg( QString::fromLatin1(mimetype) ).arg( QString::fromLatin1(resource) ).arg( QString::fromLatin1(sessionId) );
  Tracer::self()->signal( "NotificationManager::itemChanged", msg );
  emit itemChanged( sessionId, uid, remoteId, collection, mimetype, resource );
}

void Akonadi::NotificationManager::slotItemRemoved( const QByteArray &sessionId, int uid, const QString &remoteId,
                                                    int collection, const QByteArray &mimetype,
                                                    const QByteArray &resource )
{
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4, Session: %5" )
      .arg( uid ).arg( collection ).arg( QString::fromLatin1(mimetype) ).arg( QString::fromLatin1(resource) ).arg( QString::fromLatin1(sessionId) );
  Tracer::self()->signal( "NotificationManager::itemRemoved", msg );
  emit itemRemoved( sessionId, uid, remoteId, collection, mimetype, resource );
}

void Akonadi::NotificationManager::slotCollectionAdded(const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray &resource)
{
  Tracer::self()->signal( "NotificationManager::collectionAdded", QLatin1String("Location: ")
      + QString::number( collection ) + QLatin1String(" Resource: " + resource) );
  emit collectionAdded( sessionId, collection, remoteId, resource );
}

void Akonadi::NotificationManager::slotCollectionChanged(const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray & resource)
{
  Tracer::self()->signal( "NotificationManager::collectionChanged", QLatin1String("Location: ")
      + QString::number( collection ) + QLatin1String(" Resource: " + resource) );
  emit collectionChanged( sessionId, collection, remoteId, resource );
}

void Akonadi::NotificationManager::slotCollectionRemoved(const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray &resource)
{
  Tracer::self()->signal( "NotificationManager::collectionRemoved", QLatin1String("Location: ")
      + QString::number( collection ) + QLatin1String(" Resource: " + resource) );
  emit collectionRemoved( sessionId, collection, remoteId, resource );
}

#include "notificationmanager.moc"
