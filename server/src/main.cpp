/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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


#include "akonadi.h"
#include "akapplication.h"
#include "akdebug.h"
#include "akcrash.h"

#include "protocol_p.h"

#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>

#include <cstdlib>

#ifndef _WIN32_WCE
namespace po = boost::program_options;
#endif

void shutdownHandler( int )
{
  qDebug( "Shutting down AkonadiServer..." );

  Akonadi::AkonadiServer::instance()->quit();

  exit( 255 );
}

int main( int argc, char ** argv )
{
    Q_INIT_RESOURCE( akonadidb );
    AkApplication app( argc, argv );
    app.setDescription( QLatin1String( "Akonadi Server\nDo not run manually, use 'akonadictl' instead to start/stop Akonadi." ) );

#if !defined(NDEBUG) && !defined(_WIN32_WCE)
    po::options_description debugOptions( "Debug options (use with care)" );
    debugOptions.add_options()
        ( "start-without-control", "Allow to start the Akonadi server even without the Akonadi control process being available" );
    app.addCommandLineOptions( debugOptions );
#endif

    app.parseCommandLine();
    
    //Needed for wince build
    #undef interface

#ifndef _WIN32_WCE
   if ( !app.commandLineArguments().count( "start-without-control" ) &&
#else
   if (
#endif
        !QDBusConnection::sessionBus().interface()->isServiceRegistered( QLatin1String(AKONADI_DBUS_CONTROL_SERVICE_LOCK) ) ) {
     akError() << "Akonadi control process not found - aborting.";
     akFatal() << "If you started akonadiserver manually, try 'akonadictl start' instead.";
   }

    Akonadi::AkonadiServer::instance(); // trigger singleton creation
    AkonadiCrash::setShutdownMethod( shutdownHandler );

    if ( !QDBusConnection::sessionBus().registerService( QLatin1String(AKONADI_DBUS_SERVER_SERVICE) ) )
      akFatal() << "Unable to connect to dbus service: " << QDBusConnection::sessionBus().lastError().message();

    const int result = app.exec();

    Akonadi::AkonadiServer::instance()->quit();

    Q_CLEANUP_RESOURCE( akonadidb );
    return result;
}
