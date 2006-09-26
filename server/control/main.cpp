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

#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

#include "agentmanager.h"
#include "processcontrol.h"
#include "profilemanager.h"

#include "kcrash.h"
#include <stdlib.h>

static AgentManager *sAgentManager = 0;

void crashHandler( int )
{
  if ( sAgentManager )
    sAgentManager->cleanup();

  exit( 255 );
}

int main( int argc, char **argv )
{
  KCrash::init();
  KCrash::setEmergencyMethod( crashHandler );

  QCoreApplication app( argc, argv );

  if ( !QDBusConnection::sessionBus().registerService( "org.kde.Akonadi.Control" ) ) {
    qDebug( "Unable to register service: %s", qPrintable( QDBusConnection::sessionBus().lastError().message() ) );
    return 1;
  }

  sAgentManager = new AgentManager;
  ProfileManager profileManager;

  int retval = app.exec();

  delete sAgentManager;
  sAgentManager = 0;

  return retval;
}
