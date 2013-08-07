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

NotificationManager* NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  NotificationMessage::registerDBusTypes();
  NotificationMessageV2::registerDBusTypes();

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
  connect( collector, SIGNAL(notify(Akonadi::NotificationMessageV2::List)),
           SLOT(slotNotify(Akonadi::NotificationMessageV2::List)) );
}

void NotificationManager::slotNotify(const Akonadi::NotificationMessageV2::List &msgs)
{
  //akDebug() << Q_FUNC_INFO << "Appending" << msgs.count() << "notifications to current list of " << mNotifications.count() << "notifications";
  Q_FOREACH ( const NotificationMessageV2 &msg, msgs )
    NotificationMessageV2::appendAndCompress( mNotifications, msg );
  //akDebug() << Q_FUNC_INFO << "We have" << mNotifications.count() << "notifications queued in total after appendAndCompress()";

  if ( !mTimer.isActive() )
    mTimer.start();
}

QSet< NotificationSource* > NotificationManager::findInterestedSources( const NotificationMessageV2 &msg )
{
  QSet<NotificationSource*> sources;

  if ( msg.entities().count() == 0 ) {
    return sources;
  }

  if ( msg.type() == NotificationMessageV2::InvalidType ) {
    akDebug() << "Received invalid change notification";
    return sources;
  }

  sources.unite( mAllMonitored );
  if ( msg.operation() == NotificationMessageV2::Move ) {
    sources.unite( mMonitoredResources.values( msg.destinationResource() ).toSet() );
  }

  qDebug() << msg.toString();
  switch ( msg.type() ) {
    case NotificationMessageV2::InvalidType:
      return sources;

    case NotificationMessageV2::Items: {
      const QList<NotificationMessageV2::Entity> entities = msg.entities().values();

      sources.unite( mMonitoredResources.values( msg.resource() ).toSet() );
      if ( msg.operation() == NotificationMessageV2::Move ) {
        sources.unite( mMonitoredResources.values( msg.destinationResource() ) .toSet() );
      }

      if ( !mMonitoredMimeTypes.isEmpty() ) {
        Q_FOREACH ( const NotificationMessageV2::Entity &entity, entities ) {
          sources.unite( mMonitoredMimeTypes.values( entity.mimeType ).toSet() );
        }
      }

      if ( !mMonitoredItems.isEmpty() ) {
        Q_FOREACH ( const NotificationMessageV2::Entity &entity, entities ) {
          sources.unite( mMonitoredMimeTypes.values( entity.mimeType ).toSet() );
        }
      }

      sources.unite( mMonitoredCollections.values( static_cast<Entity::Id>( msg.parentCollection() ) ).toSet() );
      sources.unite( mMonitoredCollections.values( static_cast<Entity::Id>( msg.parentDestCollection() ) ).toSet() );
      // If the resource is watching root collection, then it wants to be notified
      // about all changes in it's subcollections
      sources.unite( mMonitoredCollections.values( static_cast<Entity::Id>( 0 ) ).toSet() );

    } break;

    case NotificationMessageV2::Collections: {
      const QList<NotificationMessageV2::Id> ids = msg.entities().uniqueKeys();
      sources.unite( mMonitoredResources.values( msg.resource() ).toSet() );
      if ( msg.operation() == NotificationMessageV2::Move ) {
        sources.unite( mMonitoredResources.values( msg.destinationResource() ).toSet() );
      }

      if ( !mMonitoredCollections.isEmpty() ) {
        Q_FOREACH ( NotificationMessageV2::Id id, ids ) {
          sources.unite( mMonitoredCollections.values( static_cast<Entity::Id>( id ) ).toSet() );
        }

        sources.unite( mMonitoredCollections.values( static_cast<Entity::Id>( msg.parentCollection() ) ).toSet() );
        sources.unite( mMonitoredCollections.values( static_cast<Entity::Id>( msg.parentDestCollection() ) ).toSet() );
        sources.unite( mMonitoredCollections.values( static_cast<Entity::Id>( 0 ) ).toSet() );
      }

    } break;
  }

  return sources;
}

void NotificationManager::emitPendingNotifications()
{
  if ( mNotifications.isEmpty() )
    return;

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
    Q_FOREACH ( NotificationSource *source, mClientSideMonitoredSources ) {
      source->emitNotification( mNotifications );
    }

    Q_FOREACH ( const NotificationMessageV2 &notification, mNotifications ) {

      QSet<NotificationSource*> sources = findInterestedSources( notification );

      QList<NotificationSource*> ignoredSources = mIgnoredSessions.values( notification.sessionId() );
      Q_FOREACH ( NotificationSource *source, sources ) {
        if ( !ignoredSources.contains( source ) ) {
          source->emitNotification( NotificationMessageV2::List() << notification );
        }
      }
    }
  }

  // backward compatibility with the old non-subcription interface
  // FIXME: Can we drop this already?
  if ( !legacyNotifications.isEmpty() )
    Q_EMIT notify( legacyNotifications );

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

  registerSource( source, serverSideMonitor );

  Q_EMIT subscribed( identifier );

  return source->dbusPath();
}

void NotificationManager::registerSource( NotificationSource* source,
                                          bool serverSideMonitor )
{
  mNotificationSources.insert( source->identifier(), source );
  if ( !serverSideMonitor && !mClientSideMonitoredSources.contains( source ) ) {
    mClientSideMonitoredSources.insert( source );
  }
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
  mClientSideMonitoredSources.remove( source );
  mAllMonitored.remove( source );
  Q_FOREACH ( const QByteArray &key, mIgnoredSessions.keys( source ) ) {
    mIgnoredSessions.remove( key, source );
  }
  Q_FOREACH ( const QByteArray &resource, mMonitoredResources.keys( source ) ) {
    mMonitoredResources.remove( resource, source );
  }
  Q_FOREACH ( const QString &mimeType, mMonitoredMimeTypes.keys( source ) ) {
    mMonitoredMimeTypes.remove( mimeType, source );
  }
  Q_FOREACH ( Entity::Id id, mMonitoredItems.keys( source ) ) {
    mMonitoredItems.remove( id, source );
  }
  Q_FOREACH ( Entity::Id id, mMonitoredCollections.keys( source ) ) {
    mMonitoredCollections.remove( id, source );
  }
}


QStringList NotificationManager::subscribers() const
{
  QStringList identifiers;
  Q_FOREACH ( NotificationSource *source, mNotificationSources ) {
    identifiers << source->identifier();
  }

  return identifiers;
}


