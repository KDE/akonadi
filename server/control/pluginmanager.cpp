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

#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QSettings>

#include <config-prefix.h> // for AKONADIDIR

#include "pluginmanager.h"
#include "processcontrol.h"

PluginManager::PluginManager( QObject *parent )
  : QObject( parent )
{
  mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBus::sessionBus(), this );

  QFileSystemWatcher *watcher = new QFileSystemWatcher( this );
  watcher->addPath( pluginInfoPath() );

  connect( watcher, SIGNAL( directoryChanged( const QString& ) ),
           this, SLOT( updatePluginInfos() ) );

  updatePluginInfos();

  load();
}

PluginManager::~PluginManager()
{
}

QStringList PluginManager::agentTypes() const
{
  return mPluginInfos.keys();
}

QString PluginManager::agentName( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "akonadi_control::agentName" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  return mPluginInfos[ identifier ].name;
}

QString PluginManager::agentComment( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "akonadi_control::agentComment" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  return mPluginInfos[ identifier ].comment;
}

QString PluginManager::agentIcon( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "akonadi_control::agentIcon" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  return mPluginInfos[ identifier ].icon;
}

QStringList PluginManager::agentMimeTypes( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "akonadi_control::agentMimeTypes" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QStringList();
  }

  return mPluginInfos[ identifier ].mimeTypes;
}

QStringList PluginManager::agentCapabilities( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "akonadi_control::agentCapabilities" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QStringList();
  }

  return mPluginInfos[ identifier ].capabilities;
}

QString PluginManager::createAgentInstance( const QString &identifier )
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "akonadi_control::PluginManager::createAgentInstance" ),
                      QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  /**
   * If this is the first instance of this agent type, we
   * set the instance counter to 0
   */
  if ( !mInstanceInfos.contains( identifier ) ) {
    InstanceInfo info;
    info.instanceCounter = 0;

    mInstanceInfos.insert( identifier, info );
  }

  const uint instanceCounter = mInstanceInfos[ identifier ].instanceCounter;
  mInstanceInfos[ identifier ].instanceCounter++;

  const QString agentIdentifier = QString( "%1_%2" ).arg( identifier, QString::number( instanceCounter ) );
    mTracer->warning( QLatin1String( "akonadi_control::PluginManager::createAgentInstance" ),
                      QString( "Agent identifier = %1" ).arg( agentIdentifier ) );

  Instance instance;
  instance.agentType = identifier;
  instance.controller = new Akonadi::ProcessControl( this );

  mInstances.insert( agentIdentifier, instance );

  QStringList arguments;
  arguments << "--identifier" << agentIdentifier;

  const QString executable = mPluginInfos[ identifier ].exec;
  mInstances[ agentIdentifier ].controller->start( executable, arguments );

  save();

  return agentIdentifier;
}

void PluginManager::removeAgentInstance( const QString &identifier )
{
  if ( !mInstances.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "akonadi_control::PluginManager::removeAgentInstance" ),
                      QString( "Agent instance with identifier '%1' does not exist" ).arg( identifier ) );
    return;
  }

  if ( mInstances[ identifier ].controller ) {
    mInstances[ identifier ].controller->stop();
    delete mInstances.value( identifier ).controller;
  }

  mInstances.remove( identifier );

  save();
}

QStringList PluginManager::agentInstances() const
{
  return mInstances.keys();
}

void PluginManager::updatePluginInfos()
{
  mPluginInfos.clear();

  QDir directory( pluginInfoPath(), "*.desktop" );

  QStringList files = directory.entryList();
  for ( int i = 0; i < files.count(); ++i ) {
    const QString fileName = directory.absoluteFilePath( files[ i ] );

    QSettings file( fileName, QSettings::IniFormat );
    file.beginGroup( "Desktop Entry" );

    PluginInfo info;
    info.name = file.value( "Name" ).toString();
    info.comment = file.value( "Comment" ).toString();
    info.icon = file.value( "Icon" ).toString();
    info.mimeTypes = file.value( "X-Akonadi-MimeTypes" ).toStringList();
    info.capabilities = file.value( "X-Akonadi-Capabilities" ).toStringList();
    info.exec = file.value( "Exec" ).toString();
    const QString identifier = file.value( "X-Akonadi-Identifier" ).toString();

    file.endGroup();

    if ( identifier.isEmpty() ) {
      mTracer->error( QLatin1String( "akonadi_control::updatePluginInfos" ),
                     QString( "Agent desktop file '%1' contains empty identifier" ).arg( fileName ) );
      continue;
    }

    if ( mPluginInfos.contains( identifier ) ) {
      mTracer->error( QLatin1String( "akonadi_control::updatePluginInfos" ),
                     QString( "Duplicated agent identifier '%1' from file '%2'" ).arg( fileName, identifier ) );
      continue;
    }

    if ( info.exec.isEmpty() ) {
      mTracer->error( QLatin1String( "akonadi_control::updatePluginInfos" ),
                     QString( "Agent desktop file '%1' contains empty Exec entry" ).arg( fileName ) );
      continue;
    }


    mPluginInfos.insert( identifier, info );
  }
}

QString PluginManager::pluginInfoPath()
{
  return QString( "%1/share/apps/akonadi/agents" ).arg( AKONADIDIR );
}

QString PluginManager::configPath()
{
  const QString homePath = QDir::homePath();
  const QString akonadiHomeDir = QString( "%1/%2" ).arg( homePath, ".akonadi" );

  if ( !QDir( akonadiHomeDir ).exists() ) {
    QDir dir;
    dir.mkdir( akonadiHomeDir );
  }

  return akonadiHomeDir + "/agentsrc";
}

void PluginManager::load()
{
  mInstanceInfos.clear();

  QSettings file( configPath(), QSettings::IniFormat );
  file.beginGroup( "InstanceCounters" );

  QStringList entries = file.childGroups();
  for ( int i = 0; i < entries.count(); ++i ) {
    const QString agentType = entries[ i ];

    if ( mInstanceInfos.contains( agentType ) ) {
      mTracer->warning( QLatin1String( "akonadi_control::load" ),
                        QString( "Duplicated agent type '%1' found in agentsrc" ).arg( agentType ) );
      continue;
    }

    file.beginGroup( entries[ i ] );

    const int instanceCounter = file.value( "InstanceCounter", QVariant( 0 ) ).toInt();

    if ( !mPluginInfos.contains( agentType ) ) {
      mTracer->warning( QLatin1String( "akonadi_control::load" ),
                        QString( "Reference to unknown agent type '%1' in agentsrc" ).arg( agentType ) );
      file.endGroup();
      continue;
    }

    InstanceInfo info;
    info.instanceCounter = instanceCounter;
    mInstanceInfos.insert( agentType, info );

    file.endGroup();
  }

  file.endGroup();

  file.beginGroup( "Instances" );
  entries = file.childGroups();
  for ( int i = 0; i < entries.count(); ++i ) {
    const QString instanceIdentifier = entries[ i ];

    if ( mInstances.contains( instanceIdentifier ) ) {
      mTracer->warning( QLatin1String( "akonadi_control::PluginManager::load" ),
                        QString( "Duplicated instance identifier '%1' found in agentsrc" ).arg( instanceIdentifier ) );
      continue;
    }

    file.beginGroup( entries[ i ] );

    const QString agentType = file.value( "AgentType" ).toString();
    if ( !mPluginInfos.contains( agentType ) ) {
      mTracer->warning( QLatin1String( "akonadi_control::load" ),
                        QString( "Reference to unknown agent type '%1' in agentsrc" ).arg( agentType ) );
      file.endGroup();
      continue;
    }

    Instance instance;
    instance.agentType = agentType;
    instance.controller = 0;

    mInstances.insert( instanceIdentifier, instance );

    file.endGroup();
  }
  file.endGroup();
}

void PluginManager::save()
{
  QSettings file( configPath(), QSettings::IniFormat );

  file.clear();
  file.beginGroup( "InstanceCounters" );

  QMapIterator<QString, InstanceInfo> it( mInstanceInfos );
  while ( it.hasNext() ) {
    it.next();

    file.beginGroup( it.key() );
    file.setValue( "InstanceCounter", it.value().instanceCounter );
    file.endGroup();
  }

  file.endGroup();

  file.beginGroup( "Instances" );

  QMapIterator<QString, Instance> instanceIt( mInstances );
  while ( instanceIt.hasNext() ) {
    instanceIt.next();

    file.beginGroup( instanceIt.key() );
    file.setValue( "AgentType", instanceIt.value().agentType );
    file.endGroup();
  }

  file.endGroup();

  file.sync();
}

#include "pluginmanager.moc"
