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
#include "clientcapabilityaggregator.h"

#include <akstandarddirs.h>
#include <libs/xdgbasedirs_p.h>

#include <QtCore/QDebug>
#include <QDBusConnection>
#include <QSettings>

using namespace Akonadi;

NotificationManager *NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  NotificationMessage::registerDBusTypes();
  NotificationMessageV2::registerDBusTypes();

  new NotificationManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/notifications" ),
    this, QDBusConnection::ExportAdaptors );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/notifications/debug" ),
    this, QDBusConnection::ExportScriptableSlots );

  const QString serverConfigFile = AkStandardDirs::serverConfigFile( XdgBaseDirs::ReadWrite );
  QSettings settings( serverConfigFile, QSettings::IniFormat );

  mTimer.setInterval( settings.value( QLatin1String( "NotificationManager/Interval" ), 50 ).toInt() );
  mTimer.setSingleShot( true );
  connect( &mTimer, SIGNAL(timeout()), SLOT(emitPendingNotifications()) );
}

NotificationManager::~NotificationManager()
{
}

NotificationManager *NotificationManager::self()
{
  if ( !mSelf ) {
    mSelf = new NotificationManager();
  }

  return mSelf;
}

void NotificationManager::connectNotificationCollector( NotificationCollector *collector )
{
  connect( collector, SIGNAL(notify(Akonadi::NotificationMessageV2::List)),
           SLOT(slotNotify(Akonadi::NotificationMessageV2::List)) );
}

void NotificationManager::slotNotify( const Akonadi::NotificationMessageV2::List &msgs )
{
  //akDebug() << Q_FUNC_INFO << "Appending" << msgs.count() << "notifications to current list of " << mNotifications.count() << "notifications";
  Q_FOREACH ( const NotificationMessageV2 &msg, msgs )
    NotificationMessageV2::appendAndCompress( mNotifications, msg );
  //akDebug() << Q_FUNC_INFO << "We have" << mNotifications.count() << "notifications queued in total after appendAndCompress()";

  if ( !mTimer.isActive() ) {
    mTimer.start();
  }
}

void NotificationManager::emitPendingNotifications()
{
  if ( mNotifications.isEmpty() ) {
    return;
  }

  NotificationMessage::List legacyNotifications;
  Q_FOREACH ( const NotificationMessageV2 &notification, mNotifications ) {
    Tracer::self()->signal( "NotificationManager::notify", notification.toString() );

    if ( ClientCapabilityAggregator::minimumNotificationMessageVersion() < 2 ) {
      const NotificationMessage::List tmp = notification.toNotificationV1().toList();
      Q_FOREACH ( const NotificationMessage &legacyNotification, tmp ) {
        bool appended = false;
        NotificationMessage::appendAndCompress( legacyNotifications, legacyNotification, &appended );
        if ( !appended ) {
          legacyNotifications << legacyNotification;
        }
      }
    }
  }

  if ( !legacyNotifications.isEmpty() ) {
    Q_FOREACH ( NotificationSource *src, mNotificationSources ) {
      src->emitNotification( legacyNotifications );
    }
  }

  if ( ClientCapabilityAggregator::maximumNotificationMessageVersion() > 1 ) {
    Q_FOREACH ( NotificationSource *source, mNotificationSources ) {
      if ( !source->isServerSideMonitorEnabled() ) {
        source->emitNotification( mNotifications );
        continue;
      }

      NotificationMessageV2::List acceptedNotifications;
      Q_FOREACH ( const NotificationMessageV2 &notification, mNotifications ) {
        if ( source->acceptsNotification( notification ) ) {
          acceptedNotifications << notification;
        }
      }

      if ( !acceptedNotifications.isEmpty() ) {
        source->emitNotification( acceptedNotifications );
      }
    }
  }

  // backward compatibility with the old non-subcription interface
  // FIXME: Can we drop this already?
  if ( !legacyNotifications.isEmpty() ) {
    Q_EMIT notify( legacyNotifications );
  }

  mNotifications.clear();
}

QDBusObjectPath NotificationManager::subscribeV2( const QString &identifier, bool serverSideMonitor )
{
  akDebug() << Q_FUNC_INFO << this << identifier << serverSideMonitor;

  NotificationSource *source = mNotificationSources.value( identifier );
  if ( source ) {
    akDebug() << "Known subscriber" << identifier << "subscribes again";
    source->addClientServiceName( message().service() );
  } else {
    source = new NotificationSource( identifier, message().service(), this );
  }

  registerSource( source );
  source->setServerSideMonitorEnabled( serverSideMonitor );

  Q_EMIT subscribed( identifier );

  return source->dbusPath();
}

void NotificationManager::registerSource( NotificationSource *source )
{
  mNotificationSources.insert( source->identifier(), source );
}

QDBusObjectPath NotificationManager::subscribe( const QString &identifier )
{
  akDebug() << Q_FUNC_INFO << this << identifier;
  return subscribeV2( identifier, false );
}

void NotificationManager::unsubscribe( const QString &identifier )
{
  NotificationSource *source = mNotificationSources.value( identifier );
  if ( source ) {
    unregisterSource( source );
    source->deleteLater();
    Q_EMIT unsubscribed( identifier );
  } else {
    akDebug() << "Attempt to unsubscribe unknown subscriber" << identifier;
  }
}

void NotificationManager::unregisterSource( NotificationSource *source )
{
  mNotificationSources.remove( source->identifier() );
}

QStringList NotificationManager::subscribers() const
{
  QStringList identifiers;
  Q_FOREACH ( NotificationSource *source, mNotificationSources ) {
    identifiers << source->identifier();
  }

  return identifiers;
}
