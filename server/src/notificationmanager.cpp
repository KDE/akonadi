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

#include "notificationmanager.h"
#include "notificationmanageradaptor.h"
#include "tracer.h"
#include "storage/datastore.h"

using namespace Akonadi;

NotificationManager* NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  new NotificationManagerAdaptor( this );

  QDBusConnection::sessionBus().registerObject( QLatin1String("/notifications"), this, QDBusConnection::ExportAdaptors );
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
  connect( store->notificationCollector(), SIGNAL(itemAddedNotification(int,QString,QByteArray,QByteArray)),
           SLOT(slotItemAdded(int,QString,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(itemChangedNotification(int,QString,QByteArray,QByteArray)),
           SLOT(slotItemChanged(int,QString,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(itemRemovedNotification(int,QString,QByteArray,QByteArray)),
           SLOT(slotItemRemoved(int,QString,QByteArray,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionAddedNotification(QString,QByteArray)),
           SLOT(slotCollectionAdded(QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionChangedNotification(QString,QByteArray)),
           SLOT(slotCollectionChanged(QString,QByteArray)) );
  connect( store->notificationCollector(), SIGNAL(collectionRemovedNotification(QString,QByteArray)),
           SLOT(slotCollectionRemoved(QString,QByteArray)) );
}


void Akonadi::NotificationManager::slotItemAdded( int uid, const QString &location,
                                                  const QByteArray &mimeType, const QByteArray &resource )
{
  QByteArray id = QByteArray::number( uid );
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4" )
      .arg( uid ).arg( location ).arg( QString::fromLatin1(mimeType) ).arg( QString::fromLatin1(resource) );
  Tracer::self()->signal( "NotificationManager::itemAdded", msg );
  emit itemAdded( id, location, mimeType, resource );
}

void Akonadi::NotificationManager::slotItemChanged( int uid, const QString &location,
                                                    const QByteArray &mimetype, const QByteArray &resource )
{
  QByteArray id = QByteArray::number( uid );
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4" )
      .arg( uid ).arg( location ).arg( QString::fromLatin1(mimetype) ).arg( QString::fromLatin1(resource) );
  Tracer::self()->signal( "NotificationManager::itemChanged", msg );
  emit itemChanged( id, location, mimetype, resource );
}

void Akonadi::NotificationManager::slotItemRemoved( int uid, const QString &location,
                                                    const QByteArray &mimetype, const QByteArray &resource )
{
  QByteArray id = QByteArray::number( uid );
  QString msg = QString::fromLatin1("ID: %1, Location: %2, Mimetype: %3, Resource: %4" )
      .arg( uid ).arg( location ).arg( QString::fromLatin1(mimetype) ).arg( QString::fromLatin1(resource) );
  Tracer::self()->signal( "NotificationManager::itemRemoved", msg );
  emit itemRemoved( id, location, mimetype, resource );
}

void Akonadi::NotificationManager::slotCollectionAdded(const QString &path, const QByteArray &resource)
{
  Tracer::self()->signal( "NotificationManager::collectionAdded", QLatin1String("Location: ")
      + path + QLatin1String(" Resource: " + resource) );
  emit collectionAdded( path, resource );
}

void Akonadi::NotificationManager::slotCollectionChanged(const QString &path, const QByteArray & resource)
{
  Tracer::self()->signal( "NotificationManager::collectionChanged", QLatin1String("Location: ")
      + path + QLatin1String(" Resource: " + resource) );
  emit collectionChanged( path, resource );
}

void Akonadi::NotificationManager::slotCollectionRemoved(const QString &path, const QByteArray &resource)
{
  Tracer::self()->signal( "NotificationManager::collectionRemoved", QLatin1String("Location: ")
      + path + QLatin1String(" Resource: " + resource) );
  emit collectionRemoved( path, resource );
}

#include "notificationmanager.moc"
