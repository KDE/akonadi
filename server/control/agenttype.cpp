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
#include "libs/xdgbasedirs_p.h"
#include "libs/capabilities_p.h"
#include "akdebug.h"

#include <QSettings>

using namespace Akonadi;

QLatin1String AgentType::CapabilityUnique = QLatin1String( AKONADI_AGENT_CAPABILITY_UNIQUE );
QLatin1String AgentType::CapabilityResource = QLatin1String( AKONADI_AGENT_CAPABILITY_RESOURCE );
QLatin1String AgentType::CapabilityAutostart = QLatin1String( AKONADI_AGENT_CAPABILITY_AUTOSTART );
QLatin1String AgentType::CapabilityPreprocessor = QLatin1String( AKONADI_AGENT_CAPABILITY_PREPROCESSOR );
QLatin1String AgentType::CapabilitySearch = QLatin1String( AKONADI_AGENT_CAPABILITY_SEARCH );

AgentType::AgentType() :
    instanceCounter( 0 )
{
}

bool AgentType::load(const QString & fileName, AgentManager * manager)
{
  QSettings file( fileName, QSettings::IniFormat );
  file.beginGroup( "Desktop Entry" );

  foreach(const QString& key, file.allKeys() ) {
    if ( key.startsWith( QLatin1String("Name[") ) ) {
      QString lang = key.mid( 5, key.length()-6);
      name.insert( lang, QString::fromUtf8( file.value( key ).toByteArray() ) );
    } else if ( key == "Name" ) {
      name.insert( "en_US", QString::fromUtf8( file.value( "Name" ).toByteArray() ) );
    } else if ( key.startsWith( QLatin1String("Comment[") ) ) {
      QString lang = key.mid( 8, key.length()-9);
      comment.insert( lang, QString::fromUtf8( file.value( key ).toByteArray() )  );
    } else if ( key == "Comment" ) {
      comment.insert( "en_US", QString::fromUtf8( file.value( "Comment" ).toByteArray() ) );
    }
  }
  icon = file.value( "Icon" ).toString();
  mimeTypes = file.value( "X-Akonadi-MimeTypes" ).toStringList();
  capabilities = file.value( "X-Akonadi-Capabilities" ).toStringList();
  exec = file.value( "Exec" ).toString();
  identifier = file.value( "X-Akonadi-Identifier" ).toString();

  if ( file.contains( "X-Akonadi-RunInAgentServer" ) ) {
    runInAgentServer = file.value( "X-Akonadi-RunInAgentServer" ).toBool();
  } else {
    runInAgentServer = false;
  }
  
  file.endGroup();

  if ( identifier.isEmpty() ) {
    akError() << Q_FUNC_INFO << "Agent desktop file" << fileName << "contains empty identifier";
    return false;
  }
  if ( exec.isEmpty() ) {
    akError() << Q_FUNC_INFO << "Agent desktop file" << fileName << "contains empty Exec entry";
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
