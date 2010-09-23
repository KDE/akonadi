/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "agentserver.h"

#include "shared/akapplication.h"
#include "shared/akdebug.h"

#include "libs/protocol_p.h"

#include <QtGui/QApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>

int main( int argc, char ** argv )
{
  QApplication app( argc, argv );
  app.setQuitOnLastWindowClosed( false );

  //Needed for wince build
  #undef interface

  if ( !QDBusConnection::sessionBus().interface()->isServiceRegistered( QLatin1String(AKONADI_DBUS_CONTROL_SERVICE_LOCK) ) ) {
    akError() << "Akonadi control process not found - aborting.";
    akFatal() << "If you started akonadi_agent_server manually, try 'akonadictl start' instead.";
  }

  new Akonadi::AgentServer( &app );

  if ( !QDBusConnection::sessionBus().registerService( QLatin1String(AKONADI_DBUS_AGENTSERVER_SERVICE) ) )
    akFatal() << "Unable to connect to dbus service: " << QDBusConnection::sessionBus().lastError().message();

  const int result = app.exec();
  return result;
}
