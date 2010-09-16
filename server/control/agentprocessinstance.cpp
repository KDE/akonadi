/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
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
#include "agentprocessinstance.h"

#include "agenttype.h"
#include "processcontrol.h"

#include "libs/xdgbasedirs_p.h"
#include "shared/akdebug.h"

using namespace Akonadi;

AgentProcessInstance::AgentProcessInstance( AgentManager *manager )
  : AgentInstance( manager )
  , mController( 0 )
{ }

bool AgentProcessInstance::start( const AgentType &agentInfo )
{
  Q_ASSERT( !identifier().isEmpty() );
  if ( identifier().isEmpty() )
    return false;

  setAgentType( agentInfo.identifier );

  const QString executable = XdgBaseDirs::findExecutableFile( agentInfo.exec );
  if ( executable.isEmpty() ) {
    akError() << Q_FUNC_INFO << "Unable to find agent executable" << agentInfo.exec;
    return false;
  }

  mController = new Akonadi::ProcessControl( this );
  QStringList arguments;
  arguments << "--identifier" << identifier();
  mController->start( executable, arguments );
  return true;
}

void AgentProcessInstance::quit()
{
  mController->setCrashPolicy( Akonadi::ProcessControl::StopOnCrash );
  AgentInstance::quit();
}

void AgentProcessInstance::cleanup()
{
  mController->setCrashPolicy( Akonadi::ProcessControl::StopOnCrash );
  AgentInstance::cleanup();
}

void AgentProcessInstance::restartWhenIdle()
{
  if ( status() == 0 ) {
    mController->restartOnceWhenFinished();
    quit();
  }
}
