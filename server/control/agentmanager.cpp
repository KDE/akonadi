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
}

AgentManager::~AgentManager()
{
}

QStringList AgentManager::agentTypes() const
{
  return QStringList();
}

QString AgentManager::agentName( const QString &identifier ) const
{
  return QString();
}

QString AgentManager::agentComment( const QString &identifier ) const
{
  return QString();
}

QString AgentManager::agentIcon( const QString &identifier ) const
{
  return QString();
}

QStringList AgentManager::agentMimeTypes( const QString &identifier ) const
{
  return QStringList();
}

QStringList AgentManager::agentCapabilities( const QString &identifier ) const
{
  return QStringList();
}

QString AgentManager::createAgentInstance( const QString &identifier )
{
  return QString();
}

void AgentManager::removeAgentInstance( const QString &identifier )
{
}

QStringList AgentManager::profiles() const
{
  return QStringList();
}

bool AgentManager::createProfile( const QString &identifier )
{
}

void AgentManager::removeProfile( const QString &identifier )
{
}

void AgentManager::profileAddAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
}

void AgentManager::profileRemoveAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
}

QStringList AgentManager::profileAgents( const QString &identifier ) const
{
  return QStringList();
}

#include "agentmanager.moc"
