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
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <config-akonadi.h>

#include <akapplication.h>
#include <akdbus.h>
#include "protocol_p.h"

#include "controlmanagerinterface.h"
#include "akonadistarter.h"

#if defined(HAVE_UNISTD_H) && !defined(Q_WS_WIN)
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

static bool startServer()
{
  //Needed for wince build
  #undef interface
  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( AkDBus::serviceName(AkDBus::Control) )
       || QDBusConnection::sessionBus().interface()->isServiceRegistered( AkDBus::serviceName(AkDBus::Server) ) ) {
    qDebug() << "Akonadi is already running.";
    return false;
  }
  AkonadiStarter starter;
  return starter.start();
}

static bool stopServer()
{
  org::freedesktop::Akonadi::ControlManager iface( AkDBus::serviceName(AkDBus::Control),
                                                   QLatin1String("/ControlManager"),
                                                   QDBusConnection::sessionBus(), 0 );
  if ( !iface.isValid() ) {
    qDebug( "Akonadi is not running." );
    return false;
  }

  iface.shutdown();

  return true;
}

static bool statusServer()
{
  bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AkDBus::serviceName(AkDBus::Control) );
  qDebug( "Akonadi Control: %s", registered ? "running" : "stopped" );

  registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AkDBus::serviceName(AkDBus::Server) );
  qDebug( "Akonadi Server: %s", registered ? "running" : "stopped" );

  registered = QDBusConnection::sessionBus().interface()->isServiceRegistered( QLatin1String("org.kde.nepomuk.services.nepomukqueryservice") );
  if ( registered ) {
    QString backend = QLatin1String( "Unknown" );

    // check which backend is used
    QDBusInterface interface( QLatin1String("org.kde.NepomukStorage"), QLatin1String("/nepomukstorage") );
    const QDBusReply<QString> reply = interface.call( QLatin1String("usedSopranoBackend") );
    if ( reply.isValid() ) {
      const QString name = reply.value();

      if ( name == QLatin1String( "redland" ) )
        backend = QLatin1String( "Redland" );
      else if ( name == QLatin1String( "sesame2" ) )
        backend = QLatin1String( "Sesame2" );
      else if ( name == QLatin1String( "virtuosobackend" ) )
        backend = QLatin1String( "Virtuoso" );
    }

    qDebug( "Akonadi Server Search Support: available (backend: %s)", qPrintable( backend ) );
  } else {
    qDebug( "Akonadi Server Search Support: not available" );
  }

  return true;
}

int main( int argc, char **argv )
{
  AkCoreApplication app( argc, argv );
  app.setDescription( QLatin1String("Akonadi server manipulation tool\n"
      "Usage: akonadictl [command]\n\n"
      "Commands:\n"
      "  start      : Starts the Akonadi server with all its processes\n"
      "  stop       : Stops the Akonadi server and all its processes cleanly\n"
      "  restart    : Restart Akonadi server with all its processes\n"
      "  status     : Shows a status overview of the Akonadi server\n"
      "  vacuum     : Vacuum internal storage (WARNING: needs a lot of time and disk space!)\n"
      "  fsck       : Check (and attempt to fix) consistency of the internal storage (can take some time)"
  ) );

  app.parseCommandLine();

  QStringList optionsList;
  optionsList.append( QLatin1String( "start" ) );
  optionsList.append( QLatin1String( "stop" ) );
  optionsList.append( QLatin1String( "status" ) );
  optionsList.append( QLatin1String( "restart" ) );
  optionsList.append( QLatin1String( "vacuum" ) );
  optionsList.append( QLatin1String( "fsck" ) );

#ifndef _WIN32_WCE
  const QStringList arguments = QCoreApplication::instance()->arguments();
  if ( arguments.count() != 2 ) {
    app.printUsage();
    return 1;
  } else if ( !optionsList.contains( arguments[ 1 ] ) ) {
    app.printUsage();
    return 2;
  }
#else
    QStringList arguments = QCoreApplication::instance()->arguments();
    if (argc > 1) {
      if (strcmp(argv[1],"start") == 0) {
        arguments.append(QLatin1String("start"));
      } else if (strcmp(argv[1],"stop") == 0) {
        arguments.append(QLatin1String("stop"));
      } else if (strcmp(argv[1],"restart") == 0) {
        arguments.append(QLatin1String("restart"));
      } else if (strcmp(argv[1],"status") == 0) {
        arguments.append(QLatin1String("status"));
      }
    } else {
      arguments.append(QLatin1String("start"));
    }
#endif

  if ( arguments[ 1 ] == QLatin1String( "start" ) ) {
    if ( !startServer() )
      return 3;
  } else if ( arguments[ 1 ] == QLatin1String( "stop" ) ) {
    if ( !stopServer() ) 
      return 4;
    else {
    //Block until akonadi is shut down
#ifdef _WIN32_WCE
        do {
          Sleep(100000);
        } while( QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_DBUS_CONTROL_SERVICE ) );
#endif
    }
  } else if ( arguments[ 1 ] == QLatin1String( "status" ) ) {
    if ( !statusServer() )
      return 5;
  } else if ( arguments[ 1 ] == QLatin1String( "restart") ) {
      if ( !stopServer() )
        return 4;
      else {
        do {
#if defined(HAVE_UNISTD_H) && !defined(Q_WS_WIN)
          usleep(100000);
#else
          Sleep(100000);
#endif
        } while( QDBusConnection::sessionBus().interface()->isServiceRegistered( AkDBus::serviceName(AkDBus::Control) ) );
        if ( !startServer() )
          return 3;
      }
  } else if ( arguments[ 1 ] == QLatin1String( "vacuum" ) ) {
    QDBusInterface iface( AkDBus::serviceName(AkDBus::StorageJanitor), QLatin1String(AKONADI_DBUS_STORAGEJANITOR_PATH) );
    iface.call( QDBus::NoBlock, QLatin1String( "vacuum" ) );
  } else if ( arguments[ 1 ] == QLatin1String( "fsck" ) ) {
    QDBusInterface iface( AkDBus::serviceName(AkDBus::StorageJanitor), QLatin1String(AKONADI_DBUS_STORAGEJANITOR_PATH) );
    iface.call( QDBus::NoBlock, QLatin1String( "check" ) );
  }
  return 0;
}
