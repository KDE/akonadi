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
#include "agentmanageradaptor.h"

AgentManager::AgentManager( QObject *parent )
  : QObject( parent )
{
  new AgentManagerAdaptor( this );
  QDBus::sessionBus().registerObject( "/", this );

  mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBus::sessionBus(), this );
}

AgentManager::~AgentManager()
{
}

QStringList AgentManager::agentTypes() const
{
  return mPluginManager.agentTypes();
}

QString AgentManager::agentName( const QString &identifier ) const
{
  return mPluginManager.agentName( identifier );
}

QString AgentManager::agentComment( const QString &identifier ) const
{
  return mPluginManager.agentComment( identifier );
}

QString AgentManager::agentIcon( const QString &identifier ) const
{
  return mPluginManager.agentIcon( identifier );
}

QStringList AgentManager::agentMimeTypes( const QString &identifier ) const
{
  return mPluginManager.agentMimeTypes( identifier );
}

QStringList AgentManager::agentCapabilities( const QString &identifier ) const
{
  return mPluginManager.agentCapabilities( identifier );
}

QString AgentManager::createAgentInstance( const QString &identifier )
{
  return mPluginManager.createAgentInstance( identifier );
}

void AgentManager::removeAgentInstance( const QString &identifier )
{
  mPluginManager.removeAgentInstance( identifier );
}

QStringList AgentManager::agentInstances() const
{
  return mPluginManager.agentInstances();
}

bool AgentManager::requestItemDelivery( const QString &agentIdentifier, const QString &itemIdentifier,
                                        const QString &collection, int type )
{
  return mPluginManager.requestItemDelivery( agentIdentifier, itemIdentifier, collection, type );
}

QStringList AgentManager::profiles() const
{
  return mProfileManager.profiles();
}

bool AgentManager::createProfile( const QString &identifier )
{
  return mProfileManager.createProfile( identifier );
}

void AgentManager::removeProfile( const QString &identifier )
{
  mProfileManager.removeProfile( identifier );
}

bool AgentManager::profileAddAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
  return mProfileManager.profileAddAgent( profileIdentifier, agentIdentifier );
}

bool AgentManager::profileRemoveAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
  return mProfileManager.profileRemoveAgent( profileIdentifier, agentIdentifier );
}

QStringList AgentManager::profileAgents( const QString &identifier ) const
{
  return mProfileManager.profileAgents( identifier );
}

#include "agentmanager.moc"
