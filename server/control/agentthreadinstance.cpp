/*
    Copyright (c) 2010 Bertjan Broeksema <broeksema@kde.org>

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
#include "agentthreadinstance.h"

#include "agentserverinterface.h"
#include "agenttype.h"

using namespace Akonadi;

AgentThreadInstance::AgentThreadInstance( AgentManager *manager )
  : AgentInstance( manager )
{ }

bool AgentThreadInstance::start( const AgentType &agentInfo )
{
  Q_ASSERT( !identifier().isEmpty() );
  if ( identifier().isEmpty() )
    return false;

  setAgentType( agentInfo.identifier );
  mPlugin = agentInfo.exec;

  org::freedesktop::Akonadi::AgentServer agentServer( "org.freedesktop.Akonadi.AgentServer",
                                                      "/AgentServer", QDBusConnection::sessionBus() );
  // TODO: let startAgent return a bool.
  agentServer.startAgent( identifier(), agentInfo.exec );
  return true;
}

void AgentThreadInstance::restartWhenIdle()
{
  if ( status() == 0 ) {
    org::freedesktop::Akonadi::AgentServer agentServer( "org.freedesktop.Akonadi.AgentServer",
                                                        "/AgentServer", QDBusConnection::sessionBus() );
    agentServer.stopAgent( identifier() );
    agentServer.startAgent( identifier(), mPlugin );
  }
}
