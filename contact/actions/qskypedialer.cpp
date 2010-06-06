/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "qskypedialer.h"

#include <QtCore/QProcess>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <klocale.h>

#include <unistd.h>

static bool isSkypeServiceRegistered()
{
  const QLatin1String service( "com.Skype.API" );

  QDBusConnectionInterface *interface = QDBusConnection::systemBus().interface();
  if ( interface->isServiceRegistered( service ) )
    return true;

  interface = QDBusConnection::sessionBus().interface();
  if ( interface->isServiceRegistered( service ) )
    return true;

  return false;
}

static QDBusInterface* searchSkypeDBusInterface()
{
  const QLatin1String service( "com.Skype.API" );
  const QLatin1String path( "/com/Skype" );

  QDBusInterface *interface = new QDBusInterface( service, path, QString(), QDBusConnection::systemBus() );
  if ( !interface->isValid() ) {
    delete interface;
    interface = new QDBusInterface( service, path, QString(), QDBusConnection::sessionBus() );
  }

  return interface;
}

QSkypeDialer::QSkypeDialer( const QString &applicationName )
  : mApplicationName( applicationName )
{
}

bool QSkypeDialer::dialNumber( const QString &number )
{
  // first check whether dbus interface is available yet
  if ( !isSkypeServiceRegistered() ) {

    // it could be skype is not running yet, so start it now
    if ( !QProcess::startDetached( QLatin1String( "skype" ), QStringList() ) ) {
      mErrorMessage = i18n( "Unable to start skype process, check that skype executable is in your PATH variable." );
      return false;
    }

    const int runs = 100;
    for ( int i = 0; i < runs; ++i ) {
      if ( !isSkypeServiceRegistered() )
        ::sleep( 2 );
      else
        break;
    }
  }

  // check again for the dbus interface
  QDBusInterface *interface = searchSkypeDBusInterface();

  if ( !interface->isValid() ) {
    delete interface;

    mErrorMessage = i18n( "Skype Public API (D-Bus) seems to be disabled." );
    return false;
  }

  QDBusReply<QString> reply = interface->call( QLatin1String( "Invoke" ), QString::fromLatin1( "NAME %1" ).arg( mApplicationName ) );
  if ( reply.value() != QLatin1String( "OK" ) ) {
    delete interface;

    mErrorMessage = i18n( "Skype registration failed." );
    return false;
  }

  reply = interface->call( QLatin1String( "Invoke" ), QLatin1String( "PROTOCOL 1" ) );
  if ( reply.value() != QLatin1String( "PROTOCOL 1" ) ) {
    delete interface;

    mErrorMessage = i18n( "Protocol mismatch." );
    return false;
  }

  reply = interface->call( QLatin1String( "Invoke" ), QString::fromLatin1( "CALL %1" ).arg( number ) );

  delete interface;

  return true;
}

QString QSkypeDialer::errorMessage() const
{
  return mErrorMessage;
}
