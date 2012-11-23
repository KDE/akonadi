/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Michael Jansen <kde@michael-jansen>

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
#include <akdebug.h>
#include "notificationmanageradaptor.h"
#include "notificationsource.h"
#include "tracer.h"
#include "storage/datastore.h"

#include <akstandarddirs.h>
#include <libs/xdgbasedirs_p.h>

#include <QtCore/QDebug>
#include <QDBusConnection>
#include <QSettings>


using namespace Akonadi;

NotificationManager* NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  NotificationMessage::registerDBusTypes();

  new NotificationManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String("/notifications"),
    this, QDBusConnection::ExportAdaptors );
  QDBusConnection::sessionBus().registerObject( QLatin1String("/notifications/debug"),
    this, QDBusConnection::ExportScriptableSlots );

  const QString serverConfigFile = AkStandardDirs::serverConfigFile( XdgBaseDirs::ReadWrite );
  QSettings settings( serverConfigFile, QSettings::IniFormat );

  mTimer.setInterval( settings.value( QLatin1String("NotificationManager/Interval"), 50 ).toInt() );
  mTimer.setSingleShot( true );
  connect( &mTimer, SIGNAL(timeout()), SLOT(emitPendingNotifications()) );
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

void NotificationManager::connectNotificationCollector(NotificationCollector* collector)
{
  connect( collector, SIGNAL(notify(Akonadi::NotificationMessage::List)),
           SLOT(slotNotify(Akonadi::NotificationMessage::List)) );
}

void NotificationManager::slotNotify(const Akonadi::NotificationMessage::List &msgs)
{
  Q_FOREACH ( const NotificationMessage &msg, msgs )
    NotificationMessage::appendAndCompress( mNotifications, msg );
  if ( !mTimer.isActive() )
    mTimer.start();
}

void NotificationManager::emitPendingNotifications()
{
  if ( mNotifications.isEmpty() )
    return;
  Q_FOREACH ( const NotificationMessage &msg, mNotifications ) {
    Tracer::self()->signal( "NotificationManager::notify", msg.toString() );
  }

  Q_FOREACH( NotificationSource *src, mNotificationSources ) {
    src->emitNotification( mNotifications );
  }

  // backward compatibility with the old non-subcription interface
  Q_EMIT notify( mNotifications );

  mNotifications.clear();
}



QDBusObjectPath NotificationManager::subscribe( const QString &identifier )
{
  NotificationSource *source = mNotificationSources.value( identifier );
  if ( source ) {
    akDebug() << "Known subscriber" << identifier << "subscribes again";
    source->addClientServiceName( message().service() );
  } else {
    source = new NotificationSource( identifier, message().service(), this );
    mNotificationSources.insert( identifier, source );
  }
  return source->dbusPath();
}



void NotificationManager::unsubscribe( const QString &identifier )
{
  NotificationSource *source = mNotificationSources.take( identifier );
  if ( source ) {
    source->deleteLater();
  } else {
    akDebug() << "Attempt to unsubscribe unknown subscriber" << identifier;
  }
}

