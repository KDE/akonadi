/*
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

#include "notificationsource.h"
#include <akdebug.h>

#include "notificationsourceadaptor.h"
#include "notificationmanager.h"

using namespace Akonadi;


NotificationSource::NotificationSource( const QString &identifier, const QString &clientServiceName, Akonadi::NotificationManager* parent )
  : QObject( parent ),
    mManager( parent ),
    mIdentifier( identifier ),
    mDBusIdentifier( identifier ),
    mClientWatcher( 0 )
{
  new NotificationSourceAdaptor( this );

  // Clean up for dbus usage: any non-alphanumeric char should be turned into '_'
  const int len = mDBusIdentifier.length();
  for ( int i = 0; i < len; ++i ) {
    if ( !mDBusIdentifier[i].isLetterOrNumber() )
      mDBusIdentifier[i] = QLatin1Char('_');
  }

  QDBusConnection::sessionBus().registerObject(
      dbusPath().path(),
      this,
      QDBusConnection::ExportAdaptors );

  mClientWatcher = new QDBusServiceWatcher( clientServiceName, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration, this );
  connect( mClientWatcher, SIGNAL(serviceUnregistered(QString)), SLOT(serviceUnregistered(QString)) );
}



NotificationSource::~NotificationSource()
{}



QDBusObjectPath NotificationSource::dbusPath() const
{
  return QDBusObjectPath( QLatin1String( "/subscriber/" ) + mDBusIdentifier );
}



void NotificationSource::emitNotification( const NotificationMessage::List &notifications )
{
  Q_EMIT notify( notifications );
}

void NotificationSource::emitNotification( const NotificationMessageV2::List &notifications )
{
  Q_EMIT notifyV2( notifications );
}


QString NotificationSource::identifier() const
{
  return mIdentifier;
}

void NotificationSource::unsubscribe()
{
  mManager->unsubscribe( mIdentifier );
}

bool NotificationSource::isServerSideMonitorEnabled() const
{
  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  return !mgr->mClientSideMonitoredSources.contains( const_cast<NotificationSource*>( this ) );
}


void NotificationSource::addClientServiceName(const QString& clientServiceName)
{
  if ( mClientWatcher->watchedServices().contains( clientServiceName ) )
    return;
  mClientWatcher->addWatchedService( clientServiceName );
  akDebug() << Q_FUNC_INFO << "Notification source" << mIdentifier << "now serving:" << mClientWatcher->watchedServices();
}

void NotificationSource::serviceUnregistered(const QString& serviceName)
{
  mClientWatcher->removeWatchedService( serviceName );
  akDebug() << Q_FUNC_INFO << "Notification source" << mIdentifier << "now serving:" << mClientWatcher->watchedServices();
  if ( mClientWatcher->watchedServices().isEmpty() )
    unsubscribe();
}

void NotificationSource::setMonitoredCollection( Entity::Id id, bool monitored )
{
  if ( id < 0 || !isServerSideMonitorEnabled() ) {
    return;
  }

  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  if ( monitored && !mgr->mMonitoredCollections.contains( id, this ) ) {
    mgr->mMonitoredCollections.insert( id, this );
    Q_EMIT monitoredCollectionsChanged();
  } else if ( !monitored ) {
    mgr->mMonitoredCollections.remove( id, this );
    Q_EMIT monitoredCollectionsChanged();
  }
}

QVector<Entity::Id> NotificationSource::monitoredCollections() const
{
  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  return mgr->mMonitoredCollections.keys( const_cast<NotificationSource*>( this ) ).toVector();
}


void NotificationSource::setMonitoredItem( Entity::Id id, bool monitored )
{
  if ( id < 0 || !isServerSideMonitorEnabled() ) {
    return;
  }

  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  if ( monitored && !mgr->mMonitoredItems.contains( id, this ) ) {
    mgr->mMonitoredItems.insert( id, this );
    Q_EMIT monitoredItemsChanged();
  } else if ( !monitored ) {
    mgr->mMonitoredItems.remove( id, this );
    Q_EMIT monitoredItemsChanged();
  }
}

QVector<Entity::Id> NotificationSource::monitoredItems() const
{
  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  return mgr->mMonitoredItems.keys( const_cast<NotificationSource*>( this ) ).toVector();
}

void NotificationSource::setMonitoredResource( const QByteArray &resource, bool monitored )
{
  if ( !isServerSideMonitorEnabled() ) {
    return;
  }

  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  if ( monitored && !mgr->mMonitoredResources.contains( resource, this ) ) {
    mgr->mMonitoredResources.insert( resource, this );
    Q_EMIT monitoredResourcesChanged();
  } else if ( !monitored ) {
    mgr->mMonitoredResources.remove( resource, this );
    Q_EMIT monitoredResourcesChanged();
  }
}

QVector<QByteArray> NotificationSource::monitoredResources() const
{
  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  return mgr->mMonitoredResources.keys( const_cast<NotificationSource*>( this ) ).toVector();
}


void NotificationSource::setMonitoredMimeType( const QString &mimeType, bool monitored )
{
  if ( mimeType.isEmpty() || !isServerSideMonitorEnabled() ) {
    return;
  }

  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  if ( monitored && !mgr->mMonitoredMimeTypes.contains( mimeType, this ) ) {
    mgr->mMonitoredMimeTypes.insert( mimeType, this );
    Q_EMIT monitoredMimeTypesChanged();
  } else if ( !monitored ) {
    mgr->mMonitoredMimeTypes.remove( mimeType, this );
    Q_EMIT monitoredMimeTypesChanged();
  }
}

QStringList NotificationSource::monitoredMimeTypes() const
{
  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  return mgr->mMonitoredMimeTypes.keys( const_cast<NotificationSource*>( this ) );
}

void NotificationSource::setAllMonitored( bool allMonitored )
{
  if ( !isServerSideMonitorEnabled() ) {
    return;
  }

  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  if ( allMonitored && !mgr->mAllMonitored.contains( this ) ) {
    mgr->mAllMonitored.insert( this );
    Q_EMIT isAllMonitoredChanged();
  } else if ( !allMonitored ) {
    mgr->mAllMonitored.remove( this );
    Q_EMIT isAllMonitoredChanged();
  }
}

bool NotificationSource::isAllMonitored() const
{
  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  return mgr->mAllMonitored.contains( const_cast<NotificationSource*>( this ) );
}

void NotificationSource::setIgnoredSession( const QByteArray &sessionId, bool ignored )
{
  if ( !isServerSideMonitorEnabled() ) {
    return;
  }

  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  if ( ignored && !mgr->mIgnoredSessions.contains( sessionId, this ) ) {
    mgr->mIgnoredSessions.insert( sessionId, this );
    Q_EMIT ignoredSessionsChanged();
  } else if ( !ignored ) {
    mgr->mIgnoredSessions.remove( sessionId, this );
    Q_EMIT ignoredSessionsChanged();
  }
}

QVector<QByteArray> NotificationSource::ignoredSessions() const
{
  NotificationManager *mgr = qobject_cast<NotificationManager*>( parent() );

  return mgr->mIgnoredSessions.keys( const_cast<NotificationSource*>( this ) ).toVector();
}
