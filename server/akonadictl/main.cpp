/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <akonadi/private/protocol.h>

#include "controlmanagerinterface.h"
#include "akonadistarter.h"

static void printHelp( const QString &app )
{
  qDebug( "Usage: %s start|stop|status\n\n"
          "Options:\n"
          "  start      : Starts the Akonadi server with all its processes\n"
          "  stop       : Stops the Akonadi server and all its processes cleanly\n"
          "  status     : Shows a status overview of the Akonadi server"
          "", qPrintable( app ) );
}

static bool startServer()
{
  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_DBUS_CONTROL_SERVICE )
       || QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_DBUS_SERVER_SERVICE ) ) {
    qDebug() << "Akonadi is already running.";
    return false;
  }
  AkonadiStarter starter;
  return starter.start();
}

static bool stopServer()
{
  org::kde::Akonadi::ControlManager iface( AKONADI_DBUS_CONTROL_SERVICE, "/ControlManager", QDBusConnection::sessionBus(), 0 );
  iface.shutdown();

  return true;
}

static bool statusServer()
{
  bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_DBUS_CONTROL_SERVICE );
  qDebug( "Akonadi Control: %s", registered ? "running" : "stopped" );

  registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_DBUS_SERVER_SERVICE );
  qDebug( "Akonadi Server: %s", registered ? "running" : "stopped" );

  return true;
}

int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );

  QString optionsList;
  optionsList.append( QLatin1String( "start" ) );
  optionsList.append( QLatin1String( "stop" ) );
  optionsList.append( QLatin1String( "status" ) );

  const QStringList arguments = app.arguments();
  if ( arguments.count() == 1 ) {
    printHelp( arguments.first() );
    return 1;
  } else if ( !optionsList.contains( arguments[ 1 ] ) ) {
    printHelp( arguments.first() );
    return 2;
  }

  if ( arguments[ 1 ] == QLatin1String( "start" ) ) {
    if ( !startServer() )
      return 3;
  } else if ( arguments[ 1 ] == QLatin1String( "stop" ) ) {
    if ( !stopServer() )
      return 4;
  } else if ( arguments[ 1 ] == QLatin1String( "status" ) ) {
    if ( !statusServer() )
      return 5;
  }

  return 0;
}
