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

#include "libs/xdgbasedirs_p.h"
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

AgentServer::~AgentServer()
{
  qDebug() << Q_FUNC_INFO;
  quit();

  QHash<QString, QPluginLoader*>::iterator it = m_pluginLoaders.begin();
  while ( it != m_pluginLoaders.end() ) {
    it.value()->unload();
    ++it;
  }
}

bool AgentServer::started( const QString& identifier ) const
{
  return m_agents.contains( identifier );
}

void AgentServer::startAgent( const QString& identifier, const QString& fileName )
{
  const QString pluginFile = XdgBaseDirs::findPluginFile( fileName );
  if ( pluginFile.isEmpty() ) {
    qDebug() << Q_FUNC_INFO << "plugin file not found!";
    return;
  }
 
  QPluginLoader *loader = 0;
  if ( m_pluginLoaders.contains( pluginFile ) ) {
    loader = m_pluginLoaders.value( pluginFile );
  } else {
    loader = new QPluginLoader( pluginFile, this );
    if ( !loader->load() ) {
      akError() << Q_FUNC_INFO << "Failed to load agent: " << loader->errorString();
      return;
    }
    m_pluginLoaders.insert( pluginFile, loader );
  }
  Q_ASSERT( loader->isLoaded() );

  AgentThread* thread = new AgentThread( identifier, loader->instance(), this );
  m_agents.insert( identifier, thread );
  thread->start();
}

void AgentServer::stopAgent( const QString& identifier )
{
  if ( !m_agents.contains( identifier ) )
    return;

  AgentThread* thread = m_agents.take( identifier );
  thread->quit();
}

void AgentServer::quit()
{
  qDebug() << Q_FUNC_INFO;
  QMutableHashIterator<QString, AgentThread*> it( m_agents );
  while ( it.hasNext() ) {
    it.next();
    stopAgent( it.key() );
  }

  QCoreApplication::instance()->quit();
}

#include "agentserver.moc"
