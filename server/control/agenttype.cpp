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

#include "agenttype.h"
#include "agentmanager.h"
#include "tracerinterface.h"
#include "../../libs/xdgbasedirs_p.h"

#include <QSettings>

using namespace Akonadi;

QLatin1String AgentType::CapabilityUnique = QLatin1String( "Unique" );
QLatin1String AgentType::CapabilityResource = QLatin1String( "Resource" );
QLatin1String AgentType::CapabilityAutostart = QLatin1String( "Autostart" );

AgentType::AgentType() :
    instanceCounter( 0 )
{
}

bool AgentType::load(const QString & fileName, AgentManager * manager)
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
    manager->tracer()->error( QLatin1String( "AgentType::readInfo" ),
                             QString( "Agent desktop file '%1' contains empty identifier" ).arg( fileName ) );
    return false;
  }
  if ( exec.isEmpty() ) {
    manager->tracer()->error( QLatin1String( "AgentType::readInfo" ),
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

void AgentType::save( QSettings *config ) const
{
  Q_ASSERT( config );
  if ( !capabilities.contains( CapabilityUnique ) ) {
    config->setValue( QString::fromLatin1( "InstanceCounters/%1/InstanceCounter" ).arg( identifier ), instanceCounter );
  }
}
