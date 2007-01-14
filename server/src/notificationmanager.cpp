/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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
  connect( store->notificationCollector(), SIGNAL(itemAddedNotification(int,int,QString,QByteArray,QByteArray)),
           SLOT(slotItemAdded(int,int,QString,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(itemChangedNotification(int,int,QString,QByteArray,QByteArray)),
           SLOT(slotItemChanged(int,int,QString,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(itemRemovedNotification(int,int,QString,QByteArray,QByteArray)),
           SLOT(slotItemRemoved(int,int,QString,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionAddedNotification(int,QString,QByteArray)),
           SLOT(slotCollectionAdded(int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionChangedNotification(int,QString,QByteArray)),
           SLOT(slotCollectionChanged(int,QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionRemovedNotification(int,QString,QByteArray)),
           SLOT(slotCollectionRemoved(int,QString,QByteArray)) );
}


void Akonadi::NotificationManager::slotItemAdded( int sessionId, int uid, const QString &location,
                                                  const QByteArray &mimeType, const QByteArray &resource )
{
  QByteArray id = QByteArray::number( uid );
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4, Session: %5" )
      .arg( uid ).arg( location ).arg( QString::fromLatin1(mimeType) ).arg( QString::fromLatin1(resource) ).arg( sessionId );
  Tracer::self()->signal( "NotificationManager::itemAdded", msg );
  emit itemAdded( sessionId, id, location, mimeType, resource );
}

void Akonadi::NotificationManager::slotItemChanged( int sessionId, int uid, const QString &location,
                                                    const QByteArray &mimetype, const QByteArray &resource )
{
  QByteArray id = QByteArray::number( uid );
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4, Session: %5" )
      .arg( uid ).arg( location ).arg( QString::fromLatin1(mimetype) ).arg( QString::fromLatin1(resource) ).arg( sessionId );
  Tracer::self()->signal( "NotificationManager::itemChanged", msg );
  emit itemChanged( sessionId, id, location, mimetype, resource );
}

void Akonadi::NotificationManager::slotItemRemoved( int sessionId, int uid, const QString &location,
                                                    const QByteArray &mimetype, const QByteArray &resource )
{
  QByteArray id = QByteArray::number( uid );
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4, Session: %5" )
      .arg( uid ).arg( location ).arg( QString::fromLatin1(mimetype) ).arg( QString::fromLatin1(resource) ).arg( sessionId );
  Tracer::self()->signal( "NotificationManager::itemRemoved", msg );
  emit itemRemoved( sessionId, id, location, mimetype, resource );
}

void Akonadi::NotificationManager::slotCollectionAdded(int sessionId, const QString &path, const QByteArray &resource)
{
  Tracer::self()->signal( "NotificationManager::collectionAdded", QLatin1String("Location: ")
      + path + QLatin1String(" Resource: " + resource) );
  emit collectionAdded( sessionId, path, resource );
}

void Akonadi::NotificationManager::slotCollectionChanged(int sessionId, const QString &path, const QByteArray & resource)
{
  Tracer::self()->signal( "NotificationManager::collectionChanged", QLatin1String("Location: ")
      + path + QLatin1String(" Resource: " + resource) );
  emit collectionChanged( sessionId, path, resource );
}

void Akonadi::NotificationManager::slotCollectionRemoved(int sessionId, const QString &path, const QByteArray &resource)
{
  Tracer::self()->signal( "NotificationManager::collectionRemoved", QLatin1String("Location: ")
      + path + QLatin1String(" Resource: " + resource) );
  emit collectionRemoved( sessionId, path, resource );
}

#include "notificationmanager.moc"
