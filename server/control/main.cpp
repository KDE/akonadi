/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "agentmanager.h"
#include "controlmanager.h"
#include "processcontrol.h"
#include "akapplication.h"
#include "akcrash.h"
#include "akdebug.h"

#include "protocol_p.h"

#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

#include <stdlib.h>
#include <config-akonadi.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

static AgentManager *sAgentManager = 0;

void crashHandler( int )
{
  if ( sAgentManager )
    sAgentManager->cleanup();

  exit( 255 );
}

int main( int argc, char **argv )
{
  AkApplication app( argc, argv );
  app.setDescription( "Akonadi Control Process\nDo not run this manually, use 'akonadictl' instead to start/stop Akonadi." );
  app.parseCommandLine();

  if ( !QDBusConnection::sessionBus().registerService( AKONADI_DBUS_CONTROL_SERVICE ) )
    akFatal() << "Unable to register service: " << QDBusConnection::sessionBus().lastError().message();

  new ControlManager;

  sAgentManager = new AgentManager;
  AkonadiCrash::setEmergencyMethod( crashHandler );

  int retval = app.exec();

  delete sAgentManager;
  sAgentManager = 0;

  return retval;
}
