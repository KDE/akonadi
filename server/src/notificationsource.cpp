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

#include "notificationsourceadaptor.h"

#include "notificationmanager.h"

using namespace Akonadi;


NotificationSource::NotificationSource( const QString &identifier, Akonadi::NotificationManager* parent )
  : QObject( parent ),
    mManager( parent ),
    mIdentifier( identifier ),
    mDBusIdentifier( identifier )
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

  QDBusConnection::sessionBus().registerObject(
      dbusPath().path() + QLatin1String( "/debug" ),
      this,
      QDBusConnection::ExportScriptableSlots );
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



QString NotificationSource::identifier() const
{
  return mIdentifier;
}



void NotificationSource::unsubscribe()
{
  mManager->unsubscribe( mIdentifier );
}



#include "notificationsource.moc"
