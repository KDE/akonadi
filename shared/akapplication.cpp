/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "akapplication.h"
#include "akdebug.h"
#include "config-akonadi.h"

#include <QDBusConnection>
#include <QTimer>

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;


AkApplication::AkApplication(int & argc, char ** argv) :
    QCoreApplication( argc, argv ),
    mArgc( argc ),
    mArgv( argv )
{
  akInit( QString::fromLatin1( argv[ 0 ] ) );

  if ( !QDBusConnection::sessionBus().isConnected() )
    akFatal() << "D-Bus session bus is not available!";

  // there doesn't seem to be a signal to indicate that the session bus went down, so lets use polling for now
  QTimer *timer = new QTimer( this );
  connect( timer, SIGNAL(timeout()), SLOT(pollSessionBus()) );
  timer->start( 10 * 1000 );
}

void AkApplication::parseCommandLine()
{
  try {
    po::options_description desc("General options");
    desc.add_options()
      ("help", "show this help message")
      ("version","show version information");

    po::variables_map vm;
    po::store( po::parse_command_line( mArgc, mArgv, desc ), vm );
    po::notify( vm );

    if ( vm.count( "help" ) ) {
      std::cout << desc << std::endl;
      ::exit( 0 );
    }

    if ( vm.count( "version" ) ) {
      std::cout << "Akonadi " << AKONADI_VERSION_STRING << std::endl;
        ::exit( 0 );
    }
  } catch ( std::exception &e ) {
    std::cerr << "Failed to parse command line arguments: " << e.what() << std::endl;
    ::exit( 1 );
  }
}

void AkApplication::pollSessionBus() const
{
  if ( !QDBusConnection::sessionBus().isConnected() ) {
    akError() << "D-Bus session bus went down - quitting";
    quit();
  }
}

#include "akapplication.moc"
