/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "agentserver.h"
#include "agentthread.h"

#include "libs/protocol_p.h"
#include "shared/akdebug.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>
#include <QtCore/QCoreApplication>
#include <QtCore/qpluginloader.h>

using namespace Akonadi;

AgentServer::AgentServer( QObject* parent )
  : QObject( parent )
{
  QDBusConnection::sessionBus().registerObject( AKONADI_DBUS_AGENTSERVER_PATH,
                                                this, QDBusConnection::ExportScriptableSlots );
}

bool AgentServer::started( const QString& identifier ) const
{
  return m_agents.contains( identifier );
}

void AgentServer::startAgent( const QString& identifier, const QString& fileName )
{
  qDebug() << Q_FUNC_INFO << fileName << identifier;
  QPluginLoader *loader = 0;
  if ( m_pluginLoaders.contains( fileName ) ) {
    loader = m_pluginLoaders.value( fileName );
  } else {
    loader = new QPluginLoader( fileName, this );
    if ( !loader->load() ) {
      akError() << Q_FUNC_INFO << "Failed to load agent: " << loader->errorString();
      return;
    }
    m_pluginLoaders.insert( fileName, loader );
  }
  Q_ASSERT( loader->isLoaded() );

  AgentThread* thread = new AgentThread( identifier, loader->instance(), this );
  m_agents.insert( identifier, thread );
  thread->start();
}

void AgentServer::stopAgent( const QString& identifier )
{
  qDebug() << Q_FUNC_INFO << identifier;
}

void AgentServer::quit()
{
  qDebug() << Q_FUNC_INFO;
  for ( QHash<QString, AgentThread*>::const_iterator it = m_agents.constBegin(); it != m_agents.constEnd(); ++it )
    stopAgent( it.key() );
  QCoreApplication::instance()->quit();
}

#include "agentserver.moc"
