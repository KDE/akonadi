/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (c) 2007 Volker Krause <vkrause@kde.org>                    *
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
#include "processcontrol.h"
#include "serverinterface.h"
#include <akonadi/private/xdgbasedirs_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QSettings>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtCore/QDebug>

using Akonadi::ProcessControl;

AgentManager::AgentManager( QObject *parent )
  : QObject( parent )
{
  new AgentManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/AgentManager", this );

  mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBusConnection::sessionBus(), this );

  connect( QDBusConnection::sessionBus().interface(), SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
           this, SLOT( serviceOwnerChanged( const QString&, const QString&, const QString& ) ) );

  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.Akonadi" ) )
    qFatal( "akonadiserver already running!" );

  mStorageController = new Akonadi::ProcessControl;
  mStorageController->start( "akonadiserver", QStringList(), Akonadi::ProcessControl::RestartOnCrash );
}

void AgentManager::continueStartup()
{
  // prevent multiple calls in case the server has to be restarted
  static bool first = true;
  if ( !first )
    return;
  first = false;

  readPluginInfos();

  QStringList pathList = pluginInfoPathList();

  foreach ( QString path, pathList ) {
    QFileSystemWatcher *watcher = new QFileSystemWatcher( this );
    watcher->addPath( path );

    connect( watcher, SIGNAL( directoryChanged( const QString& ) ),
             this, SLOT( updatePluginInfos() ) );
  }

  load();
  foreach ( const AgentInfo info, mAgents )
    ensureAutoStart( info );
}

AgentManager::~AgentManager()
{
  cleanup();
}

void AgentManager::cleanup()
{
  foreach ( const AgentInstanceInfo info, mAgentInstances ) {
    info.controller->setCrashPolicy( ProcessControl::StopOnCrash );
    if ( info.agentControlInterface && info.agentControlInterface->isValid() )
      info.agentControlInterface->quit();
  }

  mAgentInstances.clear();

  mStorageController->setCrashPolicy( ProcessControl::StopOnCrash );
  org::kde::Akonadi::Server *serverIface = new org::kde::Akonadi::Server( "org.kde.Akonadi", "/Server",
                                                                          QDBusConnection::sessionBus(), this );
  serverIface->quit();

  delete mStorageController;
  mStorageController = 0;
}

QStringList AgentManager::agentTypes() const
{
  return mAgents.keys();
}

QString AgentManager::agentName( const QString &identifier ) const
{
  if ( !checkAgentExists( identifier ) )
    return QString();
  return mAgents.value( identifier ).name;
}

QString AgentManager::agentComment( const QString &identifier ) const
{
  if ( !checkAgentExists( identifier ) )
    return QString();
  return mAgents.value( identifier ).comment;
}

QString AgentManager::agentIcon( const QString &identifier ) const
{
  if ( !checkAgentExists( identifier ) )
    return QString();
  const AgentInfo info = mAgents.value( identifier );
  if ( !info.icon.isEmpty() )
    return info.icon;
  return "application-x-executable";
}

QStringList AgentManager::agentMimeTypes( const QString &identifier ) const
{
  if ( !checkAgentExists( identifier ) )
    return QStringList();
  return mAgents.value( identifier ).mimeTypes;
}

QStringList AgentManager::agentCapabilities( const QString &identifier ) const
{
  if ( !checkAgentExists( identifier ) )
    return QStringList();
  return mAgents.value( identifier ).capabilities;
}

QString AgentManager::createAgentInstance( const QString &identifier )
{
  if ( !checkAgentExists( identifier ) )
    return QString();
  AgentInfo agentInfo = mAgents.value( identifier );
  mAgents[ identifier ].instanceCounter++;


  AgentInstanceInfo instance;
  if ( agentInfo.capabilities.contains( AgentInfo::CapabilityUnique ) )
    instance.identifier = identifier;
  else
    instance.identifier = QString::fromLatin1( "%1_%2" ).arg( identifier, QString::number( agentInfo.instanceCounter ) );
  instance.agentType = identifier;

  if ( mAgentInstances.contains( instance.identifier ) ) {
    mTracer->warning( QLatin1String("AgentManager::createAgentInstance"),
                      QString::fromLatin1( "Cannot create another instance of agent '%1'." ).arg( identifier ) );
    return QString();
  }

  if ( !instance.start( agentInfo, this ) )
    return QString();
  mAgentInstances.insert( instance.identifier, instance );

  save();
  return instance.identifier;
}

void AgentManager::removeAgentInstance( const QString &identifier )
{
  if ( !mAgentInstances.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::removeAgentInstance" ),
                      QString( "Agent instance with identifier '%1' does not exist" ).arg( identifier ) );
    return;
  }

  AgentInstanceInfo instance = mAgentInstances.value( identifier );
  if ( instance.agentControlInterface ) {
    instance.agentControlInterface->cleanup();
  } else {
    mTracer->error( QLatin1String( "AgentManager::removeAgentInstance" ),
                    QString( "Agent instance '%1' has no interface!" ).arg( identifier ) );
  }

  delete instance.resourceInterface;
  delete instance.agentControlInterface;
  delete instance.agentStatusInterface;
  delete instance.controller;

  mAgentInstances.remove( identifier );

  save();

  emit agentInstanceRemoved( identifier );
}

QString AgentManager::agentInstanceType( const QString &identifier )
{
  if ( !mAgentInstances.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::agentInstanceType" ),
                      QString( "Agent instance with identifier '%1' does not exist" ).arg( identifier ) );
    return QString();
  }

  return mAgentInstances.value( identifier ).agentType;
}

QStringList AgentManager::agentInstances() const
{
  return mAgentInstances.keys();
}

int AgentManager::agentInstanceStatus( const QString &identifier ) const
{
  if ( !checkAgentInterfaces( identifier, QLatin1String( "agentInstanceStatus" ) ) )
    return 2;
  return mAgentInstances.value( identifier ).agentStatusInterface->status();
}

QString AgentManager::agentInstanceStatusMessage( const QString &identifier ) const
{
  if ( !checkAgentInterfaces( identifier, QLatin1String( "agentInstanceStatusMessage" ) ) )
    return QString();
  return mAgentInstances.value( identifier ).agentStatusInterface->statusMessage();
}

uint AgentManager::agentInstanceProgress( const QString &identifier ) const
{
  if ( !checkAgentInterfaces( identifier, QLatin1String( "agentInstanceProgress" ) ) )
    return 0;
  return mAgentInstances.value( identifier ).agentStatusInterface->progress();
}

QString AgentManager::agentInstanceProgressMessage( const QString &identifier ) const
{
  return QString();
}

void AgentManager::agentInstanceConfigure( const QString &identifier, qlonglong windowId )
{
  if ( !checkAgentInterfaces( identifier, "agentInstanceConfigure" ) )
    return;
  mAgentInstances.value( identifier ).agentControlInterface->configure( windowId );
}

bool AgentManager::agentInstanceOnline(const QString & identifier)
{
  if ( !checkAgentInterfaces( identifier, QLatin1String( "agentInstanceOnline" ) ) )
    return false;
  return mAgentInstances.value( identifier ).agentStatusInterface->isOnline();
}

void AgentManager::setAgentInstanceOnline(const QString & identifier, bool state )
{
  if ( !checkAgentInterfaces( identifier, QLatin1String( "setAgentInstanceOnline" ) ) )
    return;
  mAgentInstances.value( identifier ).agentStatusInterface->setOnline( state );
}

// resource specific methods //
void AgentManager::setAgentInstanceName( const QString &identifier, const QString &name )
{
  if ( !checkResourceInterface( identifier, QLatin1String( "setAgentInstanceName" ) ) )
    return;
  mAgentInstances.value( identifier ).resourceInterface->setName( name );
}

QString AgentManager::agentInstanceName( const QString &identifier ) const
{
  if ( !checkInstance( identifier ) )
    return QString();
  const AgentInstanceInfo inst = mAgentInstances.value( identifier );
  if ( inst.isResource() )
    return inst.resourceInterface->name();
  if ( !checkAgentExists( inst.agentType ) )
    return QString();
  return mAgents.value( inst.agentType ).name;
}

void AgentManager::agentInstanceSynchronize( const QString &identifier )
{
  if ( !checkResourceInterface( identifier, QLatin1String( "agentInstanceSynchronize" ) ) )
    return;
  mAgentInstances.value( identifier ).resourceInterface->synchronize();
}

void AgentManager::agentInstanceSynchronizeCollectionTree(const QString & identifier)
{
  if ( !checkResourceInterface( identifier, QLatin1String( "agentInstanceSynchronizeCollectionTree" ) ) )
    return;
  mAgentInstances.value( identifier ).resourceInterface->synchronizeCollectionTree();
}

void AgentManager::agentInstanceSynchronizeCollection(const QString & identifier, qint64 collection)
{
  if ( !checkResourceInterface( identifier, QLatin1String( "agentInstanceSynchronizeCollection" ) ) )
    return;
  mAgentInstances.value( identifier ).resourceInterface->synchronizeCollection( collection );
}

void AgentManager::updatePluginInfos()
{
  QHash<QString, AgentInfo> oldInfos = mAgents;
  readPluginInfos();

  foreach ( const AgentInfo oldInfo, oldInfos ) {
    if ( !mAgents.contains( oldInfo.identifier ) )
      emit agentTypeRemoved( oldInfo.identifier );
  }

  foreach ( const AgentInfo newInfo, mAgents ) {
    if ( !oldInfos.contains( newInfo.identifier ) ) {
      emit agentTypeAdded( newInfo.identifier );
      ensureAutoStart( newInfo );
    }
  }
}

void AgentManager::readPluginInfos()
{
  mAgents.clear();

  QStringList pathList = pluginInfoPathList();

  foreach ( QString path, pathList ) {
      QDir directory( path, "*.desktop" );
      readPluginInfos( directory );
  }
}

void AgentManager::readPluginInfos( const QDir& directory )
{
  QStringList files = directory.entryList();
  qDebug() << "PLUGINS: " << directory.canonicalPath();
  qDebug() << "PLUGINS: " << files;
  for ( int i = 0; i < files.count(); ++i ) {
    const QString fileName = directory.absoluteFilePath( files[ i ] );

    AgentInfo agentInfo;
    if ( agentInfo.load( fileName, this ) ) {
      if ( mAgents.contains( agentInfo.identifier ) ) {
        mTracer->error( QLatin1String( "AgentManager::updatePluginInfos" ),
                        QString( "Duplicated agent identifier '%1' from file '%2'" )
                            .arg( fileName, agentInfo.identifier ) );
        continue;
      }
      qDebug() << "PLUGINS inserting: " << agentInfo.identifier << agentInfo.instanceCounter << agentInfo.capabilities;
      mAgents.insert( agentInfo.identifier, agentInfo );
    }
  }
}

QStringList AgentManager::pluginInfoPathList()
{
  return Akonadi::XdgBaseDirs::findAllResourceDirs( "data", QLatin1String( "akonadi/agents" ) );
}

QString AgentManager::configPath( bool writeable )
{
  const QString configFile =
    Akonadi::XdgBaseDirs::findResourceFile( "config", QLatin1String( "akonadi/agentsrc" ) );

  if ( !writeable && !configFile.isEmpty() )
    return configFile;

  const QString configDir = Akonadi::XdgBaseDirs::saveDir( "config", "akonadi" );

  return configDir + QLatin1String( "/agentsrc" );
}

void AgentManager::load()
{
  QSettings file( configPath( false ), QSettings::IniFormat );
  file.beginGroup( "Instances" );
  QStringList entries = file.childGroups();
  for ( int i = 0; i < entries.count(); ++i ) {
    const QString instanceIdentifier = entries[ i ];

    if ( mAgentInstances.contains( instanceIdentifier ) ) {
      mTracer->warning( QLatin1String( "AgentManager::load" ),
                        QString( "Duplicated instance identifier '%1' found in agentsrc" ).arg( instanceIdentifier ) );
      continue;
    }

    file.beginGroup( entries[ i ] );

    const QString agentType = file.value( "AgentType" ).toString();
    if ( !mAgents.contains( agentType ) ) {
      mTracer->warning( QLatin1String( "AgentManager::load" ),
                        QString( "Reference to unknown agent type '%1' in agentsrc" ).arg( agentType ) );
      file.endGroup();
      continue;
    }

    AgentInstanceInfo instance;
    instance.identifier = instanceIdentifier;
    instance.agentType = agentType;
    if ( instance.start( mAgents.value( agentType ), this ) )
      mAgentInstances.insert( instanceIdentifier, instance );
    file.endGroup();
  }

  file.endGroup();
}

void AgentManager::save()
{
  QSettings file( configPath( true ), QSettings::IniFormat );

  file.clear();
  foreach ( const AgentInfo info, mAgents )
    info.save( &file );

  file.beginGroup( "Instances" );

  foreach ( const AgentInstanceInfo info, mAgentInstances ) {
    file.beginGroup( info.identifier );
    file.setValue( "AgentType", info.agentType );
    file.endGroup();
  }

  file.endGroup();

  file.sync();
}

void AgentManager::serviceOwnerChanged( const QString &name, const QString&, const QString &newOwner )
{
  if ( name == "org.kde.Akonadi" && !newOwner.isEmpty() ) {
    // server is operational, start agents
    continueStartup();
  }

  if ( name.startsWith( "org.kde.Akonadi.Agent." ) ) {
    if ( newOwner.isEmpty() )
      return;

    const QString identifier = name.mid( 22 );
    if ( !mAgentInstances.contains( identifier ) )
      return;

    const bool restarting = mAgentInstances[ identifier ].agentControlInterface != 0;
    delete mAgentInstances[ identifier ].agentControlInterface;
    delete mAgentInstances[ identifier ].agentStatusInterface;
    mAgentInstances[ identifier ].agentControlInterface = 0;
    mAgentInstances[ identifier ].agentStatusInterface = 0;

    org::kde::Akonadi::Agent::Control *agentControlIface = new org::kde::Akonadi::Agent::Control( "org.kde.Akonadi.Agent." + identifier,
        "/", QDBusConnection::sessionBus(), this );
    if ( !agentControlIface || !agentControlIface->isValid() ) {
      mTracer->error( QLatin1String( "AgentManager::resourceRegistered" ),
                      QString( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                          .arg( identifier, agentControlIface ? agentControlIface->lastError().message() : "" ) );
      return;
    }

    agentControlIface->setObjectName( identifier );
    mAgentInstances[ identifier ].agentControlInterface = agentControlIface;


    org::kde::Akonadi::Agent::Status *agentStatusIface = new org::kde::Akonadi::Agent::Status( "org.kde.Akonadi.Agent." + identifier,
        "/", QDBusConnection::sessionBus(), this );
    if ( !agentStatusIface || !agentStatusIface->isValid() ) {
      mTracer->error( QLatin1String( "AgentManager::resourceRegistered" ),
                      QString( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                          .arg( identifier, agentStatusIface ? agentStatusIface->lastError().message() : "" ) );
      return;
    }

    agentStatusIface->setObjectName( identifier );
    mAgentInstances[ identifier ].agentStatusInterface = agentStatusIface;

    connect( agentStatusIface, SIGNAL( status( int, const QString& ) ),
            this, SLOT( status( int, const QString& ) ) );
    connect( agentStatusIface, SIGNAL( percent( int ) ),
            this, SLOT( percent( int ) ) );

    if ( !restarting )
      emit agentInstanceAdded( identifier );
  }

  else if ( name.startsWith( "org.kde.Akonadi.Resource." ) ) {
    if ( newOwner.isEmpty() )
      return;

    const QString identifier = name.mid( 25 );
    if ( !mAgentInstances.contains( identifier ) )
      return;

    delete mAgentInstances[ identifier ].resourceInterface;
    mAgentInstances[ identifier ].resourceInterface = 0;

    org::kde::Akonadi::Resource *resInterface = new org::kde::Akonadi::Resource( "org.kde.Akonadi.Resource." + identifier, "/", QDBusConnection::sessionBus(), this );

    if ( !resInterface || !resInterface->isValid() ) {
      mTracer->error( QLatin1String( "AgentManager::resourceRegistered" ),
                      QString( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                          .arg( identifier, resInterface ? resInterface->lastError().message() : "" ) );
      return;
    }

    resInterface->setObjectName( identifier );
    connect( resInterface, SIGNAL( nameChanged( const QString& ) ),
            this, SLOT( resourceNameChanged( const QString& ) ) );
    mAgentInstances[ identifier ].resourceInterface = resInterface;
  }
}

void AgentManager::status( int status, const QString &message )
{
  org::kde::Akonadi::Agent::Status *interface = static_cast<org::kde::Akonadi::Agent::Status*>( sender() );
  if ( !interface ) {
    mTracer->error( QLatin1String( "AgentManager::status" ),
                    QLatin1String( "Got signal from unknown sender" ) );
    return;
  }

  const QString identifier = interface->objectName();
  if ( identifier.isEmpty() ) {
    mTracer->error( QLatin1String( "AgentManager::status" ),
                    QLatin1String( "Sender of status signal has no identifier" ) );
    return;
  }

  emit agentInstanceStatusChanged( identifier, status, message );
}

void AgentManager::percent( int progress )
{
  org::kde::Akonadi::Agent::Status *interface = static_cast<org::kde::Akonadi::Agent::Status*>( sender() );
  if ( !interface ) {
    mTracer->error( QLatin1String( "AgentManager::percent" ),
                    QLatin1String( "Got signal from unknown sender" ) );
    return;
  }

  const QString identifier = interface->objectName();
  if ( identifier.isEmpty() ) {
    mTracer->error( QLatin1String( "AgentManager::percent" ),
                    QLatin1String( "Sender of percent signal has no identifier" ) );
    return;
  }

  emit agentInstanceProgressChanged( identifier, progress, QString() );
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
                    QLatin1String( "Sender of resourceNameChanged signal has no identifier" ) );
    return;
  }

  emit agentInstanceNameChanged( identifier, data );
}

bool AgentManager::checkInstance(const QString & identifier) const
{
  if ( !mAgentInstances.contains( identifier ) ) {
    qWarning() << "Agent instance with identifier " << identifier << " does not exist";
    return false;
  }
  return true;
}

bool AgentManager::checkResourceInterface( const QString &identifier, const QString &method ) const
{
  if ( !checkInstance( identifier ) )
    return false;
  if ( !mAgents[ mAgentInstances[ identifier ].agentType ].capabilities.contains( "Resource" ) )
    return false;
  if ( !mAgentInstances[ identifier ].resourceInterface ) {
    qWarning() << QLatin1String( "AgentManager::" ) + method << " Agent instance "
        << identifier << " has no resource interface!";
    return false;
  }

  return true;
}

bool AgentManager::checkAgentExists(const QString & identifier) const
{
  if ( !mAgents.contains( identifier ) ) {
    qWarning() << "Agent instance " << identifier << " does not exist.";
    return false;
  }
  return true;
}

bool AgentManager::checkAgentInterfaces(const QString & identifier, const QString &method) const
{
  if ( !checkInstance( identifier ) )
    return false;
  if ( !mAgentInstances.value( identifier ).agentControlInterface ) {
    qWarning() << "Agent instance (" << method << ") " << identifier << " has no agent control interface.";
    return false;
  }
  if ( !mAgentInstances.value( identifier ).agentStatusInterface ) {
    qWarning() << "Agent instance (" << method << ") " << identifier << " has no agent status interface.";
    return false;
  }
  return true;
}

void AgentManager::ensureAutoStart(const AgentInfo & info)
{
  if ( !info.capabilities.contains( AgentInfo::CapabilityAutostart ) )
    return; // no an autostart agent
  if ( mAgentInstances.contains( info.identifier ) )
    return; // already running
  AgentInstanceInfo instance;
  instance.identifier = info.identifier;
  instance.agentType = info.identifier;
  if ( instance.start( info, this ) ) {
    mAgentInstances.insert( instance.identifier, instance );
    save();
  }
}

#include "agentmanager.moc"
