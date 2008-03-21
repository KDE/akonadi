/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "agentinfo.h"
#include "agentmanager.h"
#include <akonadi/private/xdgbasedirs_p.h>
#include "processcontrol.h"

#include <QSettings>

using namespace Akonadi;

QLatin1String AgentInfo::CapabilityUnique = QLatin1String( "Unique" );
QLatin1String AgentInfo::CapabilityResource = QLatin1String( "Resource" );
QLatin1String AgentInfo::CapabilityAutostart = QLatin1String( "Autostart" );

AgentInfo::AgentInfo() :
    instanceCounter( 0 )
{
}

bool AgentInfo::load(const QString & fileName, AgentManager * manager)
{
  QSettings file( fileName, QSettings::IniFormat );
  file.beginGroup( "Desktop Entry" );

  name = file.value( "Name" ).toString();
  comment = file.value( "Comment" ).toString();
  icon = file.value( "Icon" ).toString();
  mimeTypes = file.value( "X-Akonadi-MimeTypes" ).toStringList();
  capabilities = file.value( "X-Akonadi-Capabilities" ).toStringList();
  exec = file.value( "Exec" ).toString();
  identifier = file.value( "X-Akonadi-Identifier" ).toString();
  file.endGroup();

  if ( identifier.isEmpty() ) {
    manager->tracer()->error( QLatin1String( "AgentInfo::readInfo" ),
                             QString( "Agent desktop file '%1' contains empty identifier" ).arg( fileName ) );
    return false;
  }
  if ( exec.isEmpty() ) {
    manager->tracer()->error( QLatin1String( "AgentInfo::readInfo" ),
                    QString( "Agent desktop file '%1' contains empty Exec entry" ).arg( fileName ) );
    return false;
  }

  // autostart implies unique
  if ( capabilities.contains( CapabilityAutostart ) && !capabilities.contains( CapabilityUnique ) )
    capabilities << CapabilityUnique;

  // load instance count if needed
  if ( !capabilities.contains( CapabilityUnique ) ) {
    QSettings agentrc( manager->configPath( false ), QSettings::IniFormat );
    instanceCounter = agentrc.value( QString::fromLatin1( "InstanceCounters/%1/InstanceCounter" )
        .arg( identifier ), 0 ).toInt();
  }

  return true;
}

void AgentInfo::save( QSettings *config ) const
{
  Q_ASSERT( config );
  if ( !capabilities.contains( CapabilityUnique ) ) {
    config->setValue( QString::fromLatin1( "InstanceCounters/%1/InstanceCounter" ).arg( identifier ), instanceCounter );
  }
}

AgentInstanceInfo::AgentInstanceInfo() :
    controller( 0 ),
    agentInterface( 0 ),
    resourceInterface( 0 )
{
}

bool AgentInstanceInfo::start( const AgentInfo &agentInfo, AgentManager *manager )
{
  Q_ASSERT( !identifier.isEmpty() );
  if ( identifier.isEmpty() )
    return false;
  const QString executable = Akonadi::XdgBaseDirs::findExecutableFile( agentInfo.exec );
  if ( executable.isEmpty() ) {
    manager->tracer()->error( QLatin1String( "AgentInstanceInfo::start" ),
                              QString::fromLatin1( "Unable to find agent executable '%1'" ).arg( agentInfo.exec ) );
    return false;
  }
  controller = new Akonadi::ProcessControl( manager );
  QStringList arguments;
  arguments << "--identifier" << identifier;
  controller->start( executable, arguments );
  return true;
}
