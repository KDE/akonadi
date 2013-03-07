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
  Q_EMIT notify( notifications );
}


QString NotificationSource::identifier() const
{
  return mIdentifier;
}

void NotificationSource::unsubscribe()
{
  akDebug() << Q_FUNC_INFO << mIdentifier;
  mManager->unsubscribe( mIdentifier );
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

