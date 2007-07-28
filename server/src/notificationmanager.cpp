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

#include "notificationmanager.h"
#include "notificationmanageradaptor.h"
#include "tracer.h"
#include "storage/datastore.h"

#include <QtCore/QDebug>
#include <QDBusConnection>

using namespace Akonadi;

NotificationManager* NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  NotificationMessage::registerDBusTypes();

  new NotificationManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String("/notifications"),
    this, QDBusConnection::ExportAdaptors );
  mTimer.setInterval( 50 );
  mTimer.setSingleShot( true );
  connect( &mTimer, SIGNAL(timeout()), SLOT(slotEmitNotification()) );
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
  connect( store->notificationCollector(), SIGNAL(notify(Akonadi::NotificationMessage::List)),
           SLOT(slotNotify(Akonadi::NotificationMessage::List)) );
}

void NotificationManager::slotNotify(const Akonadi::NotificationMessage::List &msgs)
{
  foreach ( const NotificationMessage msg, msgs )
    NotificationMessage::appendAndCompress( mNotifications, msg );
  if ( !mTimer.isActive() )
    mTimer.start();
}

void NotificationManager::slotEmitNotification()
{
  if ( mNotifications.isEmpty() )
    return;
  foreach ( const NotificationMessage msg, mNotifications ) {
    Tracer::self()->signal( "NotificationManager::notify", msg.toString() );
  }
  emit notify( mNotifications );
  mNotifications.clear();
}

#include "notificationmanager.moc"
