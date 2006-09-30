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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QSettings>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

#include <akonadi-prefix.h> // for AKONADIDIR

#include "agentmanager.h"
#include "agentmanageradaptor.h"
#include "processcontrol.h"

AgentManager::AgentManager( QObject *parent )
  : QObject( parent )
{
  mStorageController = new Akonadi::ProcessControl;
  // ### change back to RestartOnCrash once we don't crash anymore...
  mStorageController->start( "akonadiserver", QStringList(), Akonadi::ProcessControl::StopOnCrash );

  new AgentManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/AgentManager", this );

  mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBusConnection::sessionBus(), this );

  connect( QDBusConnection::sessionBus().interface(), SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
           this, SLOT( resourceRegistered( const QString&, const QString&, const QString& ) ) );

  readPluginInfos();

  QFileSystemWatcher *watcher = new QFileSystemWatcher( this );
  watcher->addPath( pluginInfoPath() );

  connect( watcher, SIGNAL( directoryChanged( const QString& ) ),
           this, SLOT( updatePluginInfos() ) );

  load();
}

AgentManager::~AgentManager()
{
  cleanup();
}

void AgentManager::cleanup()
{
  QMutableMapIterator<QString, Instance> it( mInstances );
  while ( it.hasNext() ) {
    it.next();

    if ( it.value().interface )
      it.value().interface->quit();
  }

  mInstances.clear();

  mStorageController->setCrashPolicy( Akonadi::ProcessControl::StopOnCrash );

  delete mStorageController;
  mStorageController = 0;
}

QStringList AgentManager::agentTypes() const
{
  return mPluginInfos.keys();
}

QString AgentManager::agentName( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::agentName" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  return mPluginInfos[ identifier ].name;
}

QString AgentManager::agentComment( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::agentComment" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  return mPluginInfos[ identifier ].comment;
}

QString AgentManager::agentIcon( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::agentIcon" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  return mPluginInfos[ identifier ].icon;
}

QStringList AgentManager::agentMimeTypes( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::agentMimeTypes" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QStringList();
  }

  return mPluginInfos[ identifier ].mimeTypes;
}

QStringList AgentManager::agentCapabilities( const QString &identifier ) const
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::agentCapabilities" ),
                     QString( "Agent type with identifier '%1' does not exist" ).arg( identifier ) );
    return QStringList();
  }

  return mPluginInfos[ identifier ].capabilities;
}

QString AgentManager::createAgentInstance( const QString &identifier )
{
  if ( !mPluginInfos.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::createAgentInstance" ),
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

  Instance instance;
  instance.agentType = identifier;
  instance.controller = new Akonadi::ProcessControl( this );
  instance.interface = 0;

  mInstances.insert( agentIdentifier, instance );

  QStringList arguments;
  arguments << "--identifier" << agentIdentifier;

  const QString executable = mPluginInfos[ identifier ].exec;
  mInstances[ agentIdentifier ].controller->start( executable, arguments );

  save();

  return agentIdentifier;
}

void AgentManager::removeAgentInstance( const QString &identifier )
{
  if ( !mInstances.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::removeAgentInstance" ),
                      QString( "Agent instance with identifier '%1' does not exist" ).arg( identifier ) );
    return;
  }

  if ( mInstances[ identifier ].interface ) {
    mInstances[ identifier ].interface->cleanup();
  } else
    mTracer->error( QLatin1String( "AgentManager::removeAgentInstance" ),
                    QString( "Agent instance '%1' has no interface!" ).arg( identifier ) );

  if ( mInstances[ identifier ].controller )
    delete mInstances.value( identifier ).controller;

  mInstances.remove( identifier );

  save();

  emit agentInstanceRemoved( identifier );
}

QString AgentManager::agentInstanceType( const QString &identifier )
{
  if ( !mInstances.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::agentInstanceType" ),
                      QString( "Agent instance with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  return mInstances[ identifier ].agentType;
}

QStringList AgentManager::agentInstances() const
{
  return mInstances.keys();
}

int AgentManager::agentInstanceStatus( const QString &identifier ) const
{
  if ( !checkInterface( identifier, QLatin1String( "agentInstanceStatus" ) ) )
    return 2;

  return mInstances[ identifier ].interface->status();
}

QString AgentManager::agentInstanceStatusMessage( const QString &identifier ) const
{
  if ( !checkInterface( identifier, QLatin1String( "agentInstanceStatusMessage" ) ) )
    return QString();

  return mInstances[ identifier ].interface->statusMessage();
}

uint AgentManager::agentInstanceProgress( const QString &identifier ) const
{
  if ( !checkInterface( identifier, QLatin1String( "agentInstanceProgress" ) ) )
    return 0;

  return mInstances[ identifier ].interface->progress();
}

QString AgentManager::agentInstanceProgressMessage( const QString &identifier ) const
{
  if ( !checkInterface( identifier, QLatin1String( "agentInstanceProgressMessage" ) ) )
    return QString();

  return mInstances[ identifier ].interface->progressMessage();
}

void AgentManager::setAgentInstanceName( const QString &identifier, const QString &name )
{
  if ( !checkInterface( identifier, QLatin1String( "setAgentInstanceName" ) ) )
    return;

  mInstances[ identifier ].interface->setName( name );
}

QString AgentManager::agentInstanceName( const QString &identifier ) const
{
  if ( !checkInterface( identifier, QLatin1String( "agentInstanceName" ) ) )
    return QString();

  return mInstances[ identifier ].interface->name();
}

void AgentManager::agentInstanceConfigure( const QString &identifier )
{
  if ( !checkInterface( identifier, QLatin1String( "agentInstanceConfigure" ) ) )
    return;

  mInstances[ identifier ].interface->configure();
}

bool AgentManager::setAgentInstanceConfiguration( const QString &identifier, const QString &data )
{
  if ( !checkInterface( identifier, QLatin1String( "setAgentInstanceConfiguration" ) ) )
    return false;

  return mInstances[ identifier ].interface->setConfiguration( data );
}

QString AgentManager::agentInstanceConfiguration( const QString &identifier ) const
{
  if ( !checkInterface( identifier, QLatin1String( "agentInstanceConfiguration" ) ) )
    return QString();

  return mInstances[ identifier ].interface->configuration();
}

void AgentManager::agentInstanceSynchronize( const QString &identifier )
{
  if ( !checkInterface( identifier, QLatin1String( "agentInstanceSynchronize" ) ) )
    return;

  mInstances[ identifier ].interface->synchronize();
}

void AgentManager::updatePluginInfos()
{
  QMap<QString, PluginInfo> oldInfos = mPluginInfos;

  readPluginInfos();

  QMapIterator<QString, PluginInfo> it( oldInfos );
  while ( it.hasNext() ) {
    it.next();

    if ( !mPluginInfos.contains( it.key() ) )
      emit agentTypeRemoved( it.key() );
  }

  it = mPluginInfos;
  while ( it.hasNext() ) {
    it.next();

    if ( !oldInfos.contains( it.key() ) )
      emit agentTypeAdded( it.key() );
  }
}

void AgentManager::readPluginInfos()
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
      mTracer->error( QLatin1String( "AgentManager::updatePluginInfos" ),
                     QString( "Agent desktop file '%1' contains empty identifier" ).arg( fileName ) );
      continue;
    }

    if ( mPluginInfos.contains( identifier ) ) {
      mTracer->error( QLatin1String( "AgentManager::updatePluginInfos" ),
                     QString( "Duplicated agent identifier '%1' from file '%2'" ).arg( fileName, identifier ) );
      continue;
    }

    if ( info.exec.isEmpty() ) {
      mTracer->error( QLatin1String( "AgentManager::updatePluginInfos" ),
                     QString( "Agent desktop file '%1' contains empty Exec entry" ).arg( fileName ) );
      continue;
    }


    mPluginInfos.insert( identifier, info );
  }
}

QString AgentManager::pluginInfoPath()
{
  return QString( "%1/share/apps/akonadi/agents" ).arg( AKONADIDIR );
}

QString AgentManager::configPath()
{
  const QString homePath = QDir::homePath();
  const QString akonadiHomeDir = QString( "%1/%2" ).arg( homePath, ".akonadi" );

  if ( !QDir( akonadiHomeDir ).exists() ) {
    QDir dir;
    dir.mkdir( akonadiHomeDir );
  }

  return akonadiHomeDir + "/agentsrc";
}

void AgentManager::load()
{
  mInstanceInfos.clear();

  QSettings file( configPath(), QSettings::IniFormat );
  file.beginGroup( "InstanceCounters" );

  QStringList entries = file.childGroups();
  for ( int i = 0; i < entries.count(); ++i ) {
    const QString agentType = entries[ i ];

    if ( mInstanceInfos.contains( agentType ) ) {
      mTracer->warning( QLatin1String( "AgentManager::load" ),
                        QString( "Duplicated agent type '%1' found in agentsrc" ).arg( agentType ) );
      continue;
    }

    file.beginGroup( entries[ i ] );

    const int instanceCounter = file.value( "InstanceCounter", QVariant( 0 ) ).toInt();

    if ( !mPluginInfos.contains( agentType ) ) {
      mTracer->warning( QLatin1String( "AgentManager::load" ),
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
      mTracer->warning( QLatin1String( "AgentManager::load" ),
                        QString( "Duplicated instance identifier '%1' found in agentsrc" ).arg( instanceIdentifier ) );
      continue;
    }

    file.beginGroup( entries[ i ] );

    const QString agentType = file.value( "AgentType" ).toString();
    if ( !mPluginInfos.contains( agentType ) ) {
      mTracer->warning( QLatin1String( "AgentManager::load" ),
                        QString( "Reference to unknown agent type '%1' in agentsrc" ).arg( agentType ) );
      file.endGroup();
      continue;
    }

    Instance instance;
    instance.agentType = agentType;
    instance.controller = new Akonadi::ProcessControl( this );
    instance.interface = 0;

    mInstances.insert( instanceIdentifier, instance );

    QStringList arguments;
    arguments << "--identifier" << instanceIdentifier;

    const QString executable = mPluginInfos[ agentType ].exec;
    mInstances[ instanceIdentifier ].controller->start( executable, arguments );

    file.endGroup();
  }

  file.endGroup();
}

void AgentManager::save()
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

bool AgentManager::requestItemDelivery( const QString & agentIdentifier, const QString & uid,
                                        const QString &remoteId, const QString & collection, int type )
{
  if ( !mInstances.contains( agentIdentifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::requestItemDelivery" ),
    QString( "Agent instance with identifier '%1' does not exist" ).arg( agentIdentifier ) );
    return false;
  }

  if ( !mInstances.value( agentIdentifier ).controller ) {
    mTracer->error( QLatin1String( "AgentManager::requestItemDelivery" ),
                    QString( "Agent instance '%1' not running!" ).arg( agentIdentifier ) );
    return false;
  }

  if ( mInstances[ agentIdentifier ].interface )
    return mInstances[ agentIdentifier ].interface->requestItemDelivery( uid, remoteId, collection, type );
  else {
    mTracer->error( QLatin1String( "AgentManager::requestItemDelivery" ),
                    QString( "Agent instance '%1' has no interface!" ).arg( agentIdentifier ) );
    return false;
  }
}

void AgentManager::resourceRegistered( const QString &name, const QString&, const QString &newOwner )
{
  if ( !name.startsWith( "org.kde.Akonadi.Resource." ) )
    return;

  const QString identifier = name.mid( 25 );

  if ( newOwner.isEmpty() ) { // interface was unregistered
    emit agentInstanceRemoved( identifier );
    return;
  }

  delete mInstances[ identifier ].interface;
  mInstances[ identifier ].interface = 0;

  org::kde::Akonadi::Resource *interface = new org::kde::Akonadi::Resource( "org.kde.Akonadi.Resource." + identifier, "/", QDBusConnection::sessionBus(), this );

  if ( !interface || !interface->isValid() ) {
    mTracer->error( QLatin1String( "AgentManager::resourceRegistered" ),
                    QString( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                        .arg( identifier, interface ? interface->lastError().message() : "" ) );
    return;
  }

  interface->setObjectName( identifier );

  connect( interface, SIGNAL( statusChanged( int, const QString& ) ),
           this, SLOT( resourceStatusChanged( int, const QString& ) ) );
  connect( interface, SIGNAL( progressChanged( uint, const QString& ) ),
           this, SLOT( resourceProgressChanged( uint, const QString& ) ) );
  connect( interface, SIGNAL( nameChanged( const QString& ) ),
           this, SLOT( resourceNameChanged( const QString& ) ) );
  connect( interface, SIGNAL( configurationChanged( const QString& ) ),
           this, SLOT( resourceConfigurationChanged( const QString& ) ) );

  mInstances[ identifier ].interface = interface;

  emit agentInstanceAdded( identifier );
}

void AgentManager::resourceStatusChanged( int status, const QString &message )
{
  org::kde::Akonadi::Resource *resource = static_cast<org::kde::Akonadi::Resource*>( sender() );
  if ( !resource ) {
    mTracer->error( QLatin1String( "AgentManager::resourceStatusChanged" ),
                    QLatin1String( "Got signal from unknown sender" ) );
    return;
  }

  const QString identifier = resource->objectName();
  if ( identifier.isEmpty() ) {
    mTracer->error( QLatin1String( "AgentManager::resourceStatusChanged" ),
                    QLatin1String( "Sender of statusChanged signal has no identifier" ) );
    return;
  }

  emit agentInstanceStatusChanged( identifier, status, message );
}

void AgentManager::resourceProgressChanged( uint progress, const QString &message )
{
  org::kde::Akonadi::Resource *resource = static_cast<org::kde::Akonadi::Resource*>( sender() );
  if ( !resource ) {
    mTracer->error( QLatin1String( "AgentManager::resourceProgressChanged" ),
                    QLatin1String( "Got signal from unknown sender" ) );
    return;
  }

  const QString identifier = resource->objectName();
  if ( identifier.isEmpty() ) {
    mTracer->error( QLatin1String( "AgentManager::resourceProgressChanged" ),
                    QLatin1String( "Sender of statusChanged signal has no identifier" ) );
    return;
  }

  emit agentInstanceProgressChanged( identifier, progress, message );
}

void AgentManager::resourceConfigurationChanged( const QString &data )
{
  org::kde::Akonadi::Resource *resource = static_cast<org::kde::Akonadi::Resource*>( sender() );
  if ( !resource ) {
    mTracer->error( QLatin1String( "AgentManager::resourceConfigurationChanged" ),
                    QLatin1String( "Got signal from unknown sender" ) );
    return;
  }

  const QString identifier = resource->objectName();
  if ( identifier.isEmpty() ) {
    mTracer->error( QLatin1String( "AgentManager::resourceConfigurationChanged" ),
                    QLatin1String( "Sender of configurationChanged signal has no identifier" ) );
    return;
  }

  emit agentInstanceConfigurationChanged( identifier, data );
}

void AgentManager::resourceNameChanged( const QString &data )
{
  org::kde::Akonadi::Resource *resource = static_cast<org::kde::Akonadi::Resource*>( sender() );
  if ( !resource ) {
    mTracer->error( QLatin1String( "AgentManager::resourceNameChanged" ),
                    QLatin1String( "Got signal from unknown sender" ) );
    return;
  }

  const QString identifier = resource->objectName();
  if ( identifier.isEmpty() ) {
    mTracer->error( QLatin1String( "AgentManager::resourceNameChanged" ),
                    QLatin1String( "Sender of configurationChanged signal has no identifier" ) );
    return;
  }

  emit agentInstanceNameChanged( identifier, data );
}

bool AgentManager::checkInterface( const QString &identifier, const QString &method ) const
{
  if ( !mInstances.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::" ) + method,
                      QString( "Agent instance with identifier '%1' does not exist" ).arg( identifier ) );
    return false;
  }

  if ( !mInstances[ identifier ].interface ) {
    mTracer->error( QLatin1String( "AgentManager::" ) + method,
                    QString( "Agent instance '%1' has no interface!" ).arg( identifier ) );
    return false;
  }

  return true;
}

#include "agentmanager.moc"
