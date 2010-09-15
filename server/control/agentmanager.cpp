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
#include "agentmanagerinternaladaptor.h"
#include "agentserverinterface.h"
#include "processcontrol.h"
#include "serverinterface.h"
#include "libs/xdgbasedirs_p.h"
#include "akdebug.h"
#include "resource_manager.h"
#include "preprocessor_manager.h"
#include "libs/protocol_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QSettings>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtCore/QDebug>

#include <boost/scoped_ptr.hpp>

using Akonadi::ProcessControl;

AgentManager::AgentManager( QObject *parent )
  : QObject( parent )
  , mAgentWatcher( new QFileSystemWatcher( this ) )
{
  new AgentManagerAdaptor( this );
  new AgentManagerInternalAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/AgentManager", this );

  connect( QDBusConnection::sessionBus().interface(), SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
           this, SLOT( serviceOwnerChanged( const QString&, const QString&, const QString& ) ) );

  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.freedesktop.Akonadi" ) )
    akFatal() << "akonadiserver already running!";

  mStorageController = new Akonadi::ProcessControl;
  connect( mStorageController, SIGNAL(unableToStart()), SLOT(serverFailure()) );
  mStorageController->start( "akonadiserver", QStringList(), Akonadi::ProcessControl::RestartOnCrash );

  mAgentServer = new Akonadi::ProcessControl;
  connect( mAgentServer, SIGNAL(unableToStart()), SLOT(agentServerFailure()) );
  mAgentServer->start( "akonadi_agent_server", QStringList(), Akonadi::ProcessControl::RestartOnCrash );
  
  connect( mAgentWatcher, SIGNAL(fileChanged(QString)), SLOT(agentExeChanged(QString)) );
}

void AgentManager::continueStartup()
{
  // prevent multiple calls in case the server has to be restarted
  static bool first = true;
  if ( !first )
    return;
  first = false;

  readPluginInfos();
  foreach ( const AgentType &info, mAgents )
    emit agentTypeAdded( info.identifier );

  const QStringList pathList = pluginInfoPathList();

  foreach ( const QString &path, pathList ) {
    QFileSystemWatcher *watcher = new QFileSystemWatcher( this );
    watcher->addPath( path );

    connect( watcher, SIGNAL( directoryChanged( const QString& ) ),
             this, SLOT( updatePluginInfos() ) );
  }

  load();
  foreach ( const AgentType &info, mAgents )
    ensureAutoStart( info );

  // register the real service name once everything is up an running
  if ( !QDBusConnection::sessionBus().registerService( AKONADI_DBUS_CONTROL_SERVICE ) ) {
    // besides a race with an older Akonadi server I have no idea how we could possibly get here...
    akFatal() << "Unable to register service as" << AKONADI_DBUS_CONTROL_SERVICE << "despite having the lock. Error was:" << QDBusConnection::sessionBus().lastError().message();
  }
  akDebug() << "Akonadi server is now operational.";
}

AgentManager::~AgentManager()
{
  cleanup();
}

void AgentManager::cleanup()
{
  foreach ( const AgentInstance::Ptr &inst, mAgentInstances )
    inst->quit();

  mAgentInstances.clear();

  mStorageController->setCrashPolicy( ProcessControl::StopOnCrash );
  org::freedesktop::Akonadi::Server *serverIface =
    new org::freedesktop::Akonadi::Server( "org.freedesktop.Akonadi", "/Server",
                                           QDBusConnection::sessionBus(), this );
  serverIface->quit();

  mAgentServer->setCrashPolicy( ProcessControl::StopOnCrash );
  org::freedesktop::Akonadi::AgentServer *agentServerIface =
    new org::freedesktop::Akonadi::AgentServer( "org.freedesktop.Akonadi.AgentServer",
                                                "/AgentServer",
                                                QDBusConnection::sessionBus(), this );
  agentServerIface->quit();

  delete mStorageController;
  mStorageController = 0;

  delete mAgentServer;
  mAgentServer = 0;
}

QStringList AgentManager::agentTypes() const
{
  return mAgents.keys();
}

QString AgentManager::agentName(  const QString &identifier, const QString& lang ) const
{
  if ( !checkAgentExists( identifier ) )
    return QString();
  const QString name = mAgents.value( identifier ).name.value( lang );
  return name.isEmpty() ? mAgents.value( identifier ).name.value( "en_US" ) : name;
}

QString AgentManager::agentComment( const QString &identifier, const QString& lang ) const
{
  if ( !checkAgentExists( identifier ) )
    return QString();
  const QString comment = mAgents.value( identifier ).comment.value( lang );
  return comment.isEmpty() ? mAgents.value( identifier ).comment.value( "en_US" ) : comment;
}

QString AgentManager::agentIcon( const QString &identifier ) const
{
  if ( !checkAgentExists( identifier ) )
    return QString();
  const AgentType info = mAgents.value( identifier );
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
  AgentType agentInfo = mAgents.value( identifier );
  mAgents[ identifier ].instanceCounter++;


  AgentInstance::Ptr instance( new AgentInstance( this ) );
  if ( agentInfo.capabilities.contains( AgentType::CapabilityUnique ) )
    instance->setIdentifier( identifier );
  else
    instance->setIdentifier( QString::fromLatin1( "%1_%2" ).arg( identifier, QString::number( agentInfo.instanceCounter ) ) );

  if ( mAgentInstances.contains( instance->identifier() ) ) {
    akError() << Q_FUNC_INFO << "Cannot create another instance of agent" << identifier;
    return QString();
  }

  // Return from this dbus call before we do the next. Otherwise dbus brakes for
  // this process.
  if ( calledFromDBus() )
    connection().send( message().createReply( instance->identifier() ) );

  if ( !instance->start( agentInfo ) )
    return QString();

  mAgentInstances.insert( instance->identifier(), instance );
  registerAgentAtServer( instance->identifier(), agentInfo );
  save();

  return instance->identifier();
}

void AgentManager::removeAgentInstance( const QString &identifier )
{
  if ( !mAgentInstances.contains( identifier ) ) {
    akError() << Q_FUNC_INFO << "Agent instance with identifier" << identifier << "does not exist";
    return;
  }

  AgentInstance::Ptr instance = mAgentInstances.value( identifier );
  if ( instance->hasAgentInterface() ) {
    instance->cleanup();
  } else {
    akError() << Q_FUNC_INFO << "Agent instance" << identifier << "has no interface!";
  }

  mAgentInstances.remove( identifier );

  save();
  
  org::freedesktop::Akonadi::ResourceManager * resmanager = new org::freedesktop::Akonadi::ResourceManager( QLatin1String("org.freedesktop.Akonadi"), QLatin1String("/ResourceManager"), QDBusConnection::sessionBus(), this );
  resmanager->removeResourceInstance(instance->identifier());

  // Kill the preprocessor instance, if any.
  org::freedesktop::Akonadi::PreprocessorManager preProcessorManager(
      QLatin1String( "org.freedesktop.Akonadi" ),
      QLatin1String( "/PreprocessorManager" ),
      QDBusConnection::sessionBus(),
      this
    );

  preProcessorManager.unregisterInstance( instance->identifier() );

  emit agentInstanceRemoved( identifier );
}

QString AgentManager::agentInstanceType( const QString &identifier )
{
  if ( !mAgentInstances.contains( identifier ) ) {
    akError() << Q_FUNC_INFO << "Agent instance with identifier" << identifier << "does not exist";
    return QString();
  }

  return mAgentInstances.value( identifier )->agentType();
}

QStringList AgentManager::agentInstances() const
{
  return mAgentInstances.keys();
}

int AgentManager::agentInstanceStatus( const QString &identifier ) const
{
  if ( !checkInstance( identifier ) )
    return 2;
  return mAgentInstances.value( identifier )->status();
}

QString AgentManager::agentInstanceStatusMessage( const QString &identifier ) const
{
  if ( !checkInstance( identifier ) )
    return QString();
  return mAgentInstances.value( identifier )->statusMessage();
}

uint AgentManager::agentInstanceProgress( const QString &identifier ) const
{
  if ( !checkInstance( identifier ) )
    return 0;
  return mAgentInstances.value( identifier )->progress();
}

QString AgentManager::agentInstanceProgressMessage( const QString &identifier ) const
{
  Q_UNUSED(identifier);

  return QString();
}

void AgentManager::agentInstanceConfigure( const QString &identifier, qlonglong windowId )
{
  if ( !checkAgentInterfaces( identifier, "agentInstanceConfigure" ) )
    return;
  mAgentInstances.value( identifier )->controlInterface()->configure( windowId );
}

bool AgentManager::agentInstanceOnline(const QString & identifier)
{
  if ( !checkInstance( identifier ) )
    return false;
  return mAgentInstances.value( identifier )->isOnline();
}

void AgentManager::setAgentInstanceOnline(const QString & identifier, bool state )
{
  if ( !checkAgentInterfaces( identifier, QLatin1String( "setAgentInstanceOnline" ) ) )
    return;
  mAgentInstances.value( identifier )->statusInterface()->setOnline( state );
}

// resource specific methods //
void AgentManager::setAgentInstanceName( const QString &identifier, const QString &name )
{
  if ( !checkResourceInterface( identifier, QLatin1String( "setAgentInstanceName" ) ) )
    return;
  mAgentInstances.value( identifier )->resourceInterface()->setName( name );
}

QString AgentManager::agentInstanceName( const QString &identifier, const QString &lang ) const
{
  if ( !checkInstance( identifier ) )
    return QString();
  const AgentInstance::Ptr inst = mAgentInstances.value( identifier );
  if ( !inst->resourceName().isEmpty() )
    return inst->resourceName();
  if ( !checkAgentExists( inst->agentType() ) )
    return QString();
  const QString name = mAgents.value( inst->agentType() ).name.value( lang );
  return name.isEmpty() ? mAgents.value( inst->agentType() ).name.value("en_US") : name;
}

void AgentManager::agentInstanceSynchronize( const QString &identifier )
{
  if ( !checkResourceInterface( identifier, QLatin1String( "agentInstanceSynchronize" ) ) )
    return;
  mAgentInstances.value( identifier )->resourceInterface()->synchronize();
}

void AgentManager::agentInstanceSynchronizeCollectionTree(const QString & identifier)
{
  if ( !checkResourceInterface( identifier, QLatin1String( "agentInstanceSynchronizeCollectionTree" ) ) )
    return;
  mAgentInstances.value( identifier )->resourceInterface()->synchronizeCollectionTree();
}

void AgentManager::agentInstanceSynchronizeCollection(const QString & identifier, qint64 collection)
{
  if ( !checkResourceInterface( identifier, QLatin1String( "agentInstanceSynchronizeCollection" ) ) )
    return;
  mAgentInstances.value( identifier )->resourceInterface()->synchronizeCollection( collection );
}

void AgentManager::restartAgentInstance(const QString& identifier)
{
  if ( !checkInstance( identifier ) )
    return;
  mAgentInstances.value( identifier )->restartWhenIdle();
}

void AgentManager::updatePluginInfos()
{
  QHash<QString, AgentType> oldInfos = mAgents;
  readPluginInfos();

  foreach ( const AgentType &oldInfo, oldInfos ) {
    if ( !mAgents.contains( oldInfo.identifier ) )
      emit agentTypeRemoved( oldInfo.identifier );
  }

  foreach ( const AgentType &newInfo, mAgents ) {
    if ( !oldInfos.contains( newInfo.identifier ) ) {
      emit agentTypeAdded( newInfo.identifier );
      ensureAutoStart( newInfo );
    }
  }
}

void AgentManager::readPluginInfos()
{
  if ( !mAgentWatcher->files().isEmpty() )
    mAgentWatcher->removePaths( mAgentWatcher->files() );
  mAgents.clear();

  QStringList pathList = pluginInfoPathList();

  foreach ( const QString &path, pathList ) {
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

    AgentType agentInfo;
    if ( agentInfo.load( fileName, this ) ) {
      if ( mAgents.contains( agentInfo.identifier ) ) {
        akError() << Q_FUNC_INFO << "Duplicated agent identifier" << agentInfo.identifier << "from file" << fileName;
        continue;
      }

      const QString disableAutostart = getEnv( "AKONADI_DISABLE_AGENT_AUTOSTART" );
      if( !disableAutostart.isEmpty() ) {
        qDebug() << "Autostarting of agents is disabled.";
        agentInfo.capabilities.removeOne( AgentType::CapabilityAutostart );
      }

      const QString executable = Akonadi::XdgBaseDirs::findExecutableFile( agentInfo.exec );
      if ( executable.isEmpty() ) {
        akError() << "Executable" << agentInfo.exec << "for agent" << agentInfo.identifier << "could not be found!";
        continue;
      }

      qDebug() << "PLUGINS inserting: " << agentInfo.identifier << agentInfo.instanceCounter << agentInfo.capabilities;
      mAgents.insert( agentInfo.identifier, agentInfo );
      if ( !mAgentWatcher->files().contains( executable) )
	mAgentWatcher->addPath( executable );
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
      akError() << Q_FUNC_INFO << "Duplicated instance identifier" << instanceIdentifier << "found in agentsrc";
      continue;
    }

    file.beginGroup( entries[ i ] );

    const QString agentTypeStr = file.value( "AgentType" ).toString();
    if ( !mAgents.contains( agentTypeStr ) ) {
      akError() << Q_FUNC_INFO << "Reference to unknown agent type" << agentTypeStr << "in agentsrc";
      file.endGroup();
      continue;
    }

    AgentType agentType = mAgents.value( agentTypeStr );
    if ( agentType.runInAgentServer ) {
      org::freedesktop::Akonadi::AgentServer agentServer( "org.freedesktop.Akonadi.AgentServer",
                                                          "/AgentServer", QDBusConnection::sessionBus() );
      agentServer.startAgent( instanceIdentifier, agentType.exec );
    } else {
      AgentInstance::Ptr instance( new AgentInstance( this ) );
      instance->setIdentifier( instanceIdentifier );
      if ( instance->start( mAgents.value( agentTypeStr ) ) )
        mAgentInstances.insert( instanceIdentifier, instance );
    }
    
    file.endGroup();
  }

  file.endGroup();
}

void AgentManager::save()
{
  QSettings file( configPath( true ), QSettings::IniFormat );

  file.clear();
  foreach ( const AgentType &info, mAgents )
    info.save( &file );

  file.beginGroup( "Instances" );

  foreach ( const AgentInstance::Ptr &inst, mAgentInstances ) {
    file.beginGroup( inst->identifier() );
    file.setValue( "AgentType", inst->agentType() );
    file.endGroup();
  }

  file.endGroup();

  file.sync();
}

void AgentManager::serviceOwnerChanged( const QString &name, const QString &, const QString &newOwner )
{
  // This is called by the D-Bus server when a service comes up, goes down or changes ownership for some reason
  // and this is where we "hook up" our different Agent interfaces.

  //qDebug() << "Service " << name << " owner changed from " << oldOwner << " to " << newOwner;

  if ( name == "org.freedesktop.Akonadi" && !newOwner.isEmpty() ) {
    // server is operational, start agents
    continueStartup();
  }

  if ( name.startsWith( QLatin1String("org.freedesktop.Akonadi.Agent.") ) ) {

    // An agent service went up or down

    if ( newOwner.isEmpty() )
      return; // It went down: we don't care here.

    const QString identifier = name.mid( 30 );
    if ( !mAgentInstances.contains( identifier ) )
      return;

    AgentInstance::Ptr instance = mAgentInstances.value( identifier );
    const bool restarting = instance->hasAgentInterface();
    if ( !instance->obtainAgentInterface() )
      return;

    if ( !restarting )
      emit agentInstanceAdded( identifier );
  }

  else if ( name.startsWith( QLatin1String("org.freedesktop.Akonadi.Resource.") ) ) {

    // A resource service went up or down

    if ( newOwner.isEmpty() )
      return; // It went down: we don't care here.

    const QString identifier = name.mid( 33 );
    if ( !mAgentInstances.contains( identifier ) )
      return;

    mAgentInstances.value( identifier )->obtainResourceInterface();
  }

  else if ( name.startsWith( QLatin1String("org.freedesktop.Akonadi.Preprocessor.") ) ) {

    // A preprocessor service went up or down

    // If the preprocessor is going up then the org.freedesktop.Akonadi.Agent.* interface
    // should be already up (as it's registered before the preprocessor one).
    // So if we don't know about the preprocessor as agent instance
    // then it's not our preprocessor.

    // If the preprocessor is going down then either the agent interface already
    // went down (and it has been already unregistered on the manager side)
    // or it's still registered as agent and WE have to unregister it.
    // The order of interface deletions depends on Qt but we handle both cases.

    // Check if we "know" about it.

    const QString identifier = name.mid( 37 );

    qDebug() << "Preprocessor " << identifier << " is going up or down...";

    if ( !mAgentInstances.contains( identifier ) )
    {
      qDebug() << "But it isn't registered as agent... not mine (anymore?)";
      return; // not our agent (?)
    }

    org::freedesktop::Akonadi::PreprocessorManager preProcessorManager(
        QLatin1String( "org.freedesktop.Akonadi" ),
        QLatin1String( "/PreprocessorManager" ),
        QDBusConnection::sessionBus(),
        this
      );

    if( !preProcessorManager.isValid() )
    {
      akError() << Q_FUNC_INFO <<"Could not connect to PreprocessorManager via D-Bus:" << preProcessorManager.lastError();
    } else {
      if ( newOwner.isEmpty() )
      {
        // The preprocessor went down. Unregister it on server side.

        preProcessorManager.unregisterInstance( identifier );

      } else {

        // The preprocessor went up. Register it on server side.

        if ( !mAgentInstances.value( identifier )->obtainPreprocessorInterface() )
        {
          // Hm.. couldn't hook up its preprocessor interface..
          // Make sure we don't have it in the preprocessor chain
          qWarning() << "Couldn't obtain preprocessor interface for instance " << identifier;
 
          preProcessorManager.unregisterInstance( identifier );
          return;
        }

        qDebug() << "Registering preprocessor instance " << identifier;

        // Add to the preprocessor chain
        preProcessorManager.registerInstance( identifier );
      }
    }
  }
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
  if ( !mAgents[ mAgentInstances[ identifier ]->agentType() ].capabilities.contains( "Resource" ) )
    return false;
  if ( !mAgentInstances[ identifier ]->hasResourceInterface() ) {
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
  if ( !mAgentInstances.value( identifier )->hasAgentInterface() ) {
    qWarning() << "Agent instance (" << method << ") " << identifier << " has no agent interface.";
    return false;
  }
  return true;
}

void AgentManager::ensureAutoStart(const AgentType & info)
{
  if ( !info.capabilities.contains( AgentType::CapabilityAutostart ) )
    return; // no an autostart agent

  org::freedesktop::Akonadi::AgentServer agentServer( "org.freedesktop.Akonadi.AgentServer",
                                                      "/AgentServer", QDBusConnection::sessionBus(), this );
    
  if ( mAgentInstances.contains( info.identifier ) || agentServer.started( info.identifier ) )
    return; // already running

  if ( info.runInAgentServer ) {
    agentServer.startAgent( info.identifier, info.exec );
    registerAgentAtServer( info.identifier, info );
    save();
  } else {
    AgentInstance::Ptr instance( new AgentInstance( this ) );
    instance->setIdentifier( info.identifier );
    if ( instance->start( info ) ) {
      mAgentInstances.insert( instance->identifier(), instance );
      registerAgentAtServer( instance->identifier(), info );
      save();
    }
  }
}

void AgentManager::agentExeChanged(const QString & fileName)
{
  if ( !QFile::exists( fileName ) )
    return;
  foreach ( const AgentType &type, mAgents ) {
    if ( fileName.endsWith( type.exec ) ) {
      foreach ( const AgentInstance::Ptr &instance, mAgentInstances ) {
        if ( instance->agentType() == type.identifier )
          instance->restartWhenIdle();
      }
    }
  }
}

void AgentManager::registerAgentAtServer( const QString &agentIdentifier, const AgentType &type )
{
  if ( type.capabilities.contains( AgentType::CapabilityResource ) ) {
    boost::scoped_ptr<org::freedesktop::Akonadi::ResourceManager> resmanager(
      new org::freedesktop::Akonadi::ResourceManager( QLatin1String("org.freedesktop.Akonadi"),
                                                      QLatin1String("/ResourceManager"),
                                                      QDBusConnection::sessionBus(), this ) );
    resmanager->addResourceInstance( agentIdentifier, type.capabilities );
  }
}


void AgentManager::addSearch(const QString& query, const QString& queryLanguage, qint64 resultCollectionId)
{
  qDebug() << "AgentManager::addSearch" << query << queryLanguage << resultCollectionId;
  foreach ( const AgentInstance::Ptr &instance, mAgentInstances ) {
    const AgentType type = mAgents.value( instance->agentType() );
    if ( type.capabilities.contains( AgentType::CapabilitySearch ) && instance->searchInterface() )
      instance->searchInterface()->addSearch( query, queryLanguage, resultCollectionId );
  }
}

void AgentManager::removeSearch(quint64 resultCollectionId)
{
  qDebug() << "AgentManager::removeSearch" << resultCollectionId;
  foreach ( const AgentInstance::Ptr &instance, mAgentInstances ) {
    const AgentType type = mAgents.value( instance->agentType() );
    if ( type.capabilities.contains( AgentType::CapabilitySearch ) && instance->searchInterface() )
      instance->searchInterface()->removeSearch( resultCollectionId );
  }
}

void AgentManager::agentServerFailure()
{
  qDebug() << "Failed to start AgentServer!";
  // if ( requiresAgentServer )
  //   QCoreApplication::instance()->exit( 255 );
}

void AgentManager::serverFailure()
{
  QCoreApplication::instance()->exit( 255 );
}

#include "agentmanager.moc"
