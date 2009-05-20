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
#include "../../libs/xdgbasedirs_p.h"
#include "akdebug.h"
#include "resource_manager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QSettings>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtCore/QDebug>

using Akonadi::ProcessControl;

AgentManager::AgentManager( QObject *parent )
  : QObject( parent ),
  mAgentWatcher( new QFileSystemWatcher( this ) )
{
  new AgentManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/AgentManager", this );

  mTracer = new org::freedesktop::Akonadi::Tracer( "org.freedesktop.Akonadi", "/tracing", QDBusConnection::sessionBus(), this );

  connect( QDBusConnection::sessionBus().interface(), SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
           this, SLOT( serviceOwnerChanged( const QString&, const QString&, const QString& ) ) );

  if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.freedesktop.Akonadi" ) )
    qFatal( "akonadiserver already running!" );

  mStorageController = new Akonadi::ProcessControl;
  connect( mStorageController, SIGNAL(unableToStart()),
           QCoreApplication::instance(), SLOT(quit()) );
  mStorageController->start( "akonadiserver", QStringList(), Akonadi::ProcessControl::RestartOnCrash );

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

  QStringList pathList = pluginInfoPathList();

  foreach ( const QString &path, pathList ) {
    QFileSystemWatcher *watcher = new QFileSystemWatcher( this );
    watcher->addPath( path );

    connect( watcher, SIGNAL( directoryChanged( const QString& ) ),
             this, SLOT( updatePluginInfos() ) );
  }

  load();
  foreach ( const AgentType &info, mAgents )
    ensureAutoStart( info );
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
    mTracer->warning( QLatin1String("AgentManager::createAgentInstance"),
                      QString::fromLatin1( "Cannot create another instance of agent '%1'." ).arg( identifier ) );
    return QString();
  }

  if ( !instance->start( agentInfo ) )
    return QString();
  mAgentInstances.insert( instance->identifier(), instance );

  org::freedesktop::Akonadi::ResourceManager * resmanager = new org::freedesktop::Akonadi::ResourceManager( QLatin1String("org.freedesktop.Akonadi"), QLatin1String("/ResourceManager"), QDBusConnection::sessionBus(), this );
  resmanager->addResourceInstance(instance->identifier());

  save();
  return instance->identifier();
}

void AgentManager::removeAgentInstance( const QString &identifier )
{
  if ( !mAgentInstances.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::removeAgentInstance" ),
                      QString( "Agent instance with identifier '%1' does not exist" ).arg( identifier ) );
    return;
  }

  AgentInstance::Ptr instance = mAgentInstances.value( identifier );
  if ( instance->hasAgentInterface() ) {
    instance->cleanup();
  } else {
    mTracer->error( QLatin1String( "AgentManager::removeAgentInstance" ),
                    QString( "Agent instance '%1' has no interface!" ).arg( identifier ) );
  }

  mAgentInstances.remove( identifier );

  save();
  
  org::freedesktop::Akonadi::ResourceManager * resmanager = new org::freedesktop::Akonadi::ResourceManager( QLatin1String("org.freedesktop.Akonadi"), QLatin1String("/ResourceManager"), QDBusConnection::sessionBus(), this );
  resmanager->removeResourceInstance(instance->identifier());

  emit agentInstanceRemoved( identifier );
}

QString AgentManager::agentInstanceType( const QString &identifier )
{
  if ( !mAgentInstances.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "AgentManager::agentInstanceType" ),
                      QString( "Agent instance with identifier '%1' does not exist" ).arg( identifier ) );
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

QString AgentManager::agentInstanceName( const QString &identifier ) const
{
  if ( !checkInstance( identifier ) )
    return QString();
  const AgentInstance::Ptr inst = mAgentInstances.value( identifier );
  if ( !inst->resourceName().isEmpty() )
    return inst->resourceName();
  if ( !checkAgentExists( inst->agentType() ) )
    return QString();
  return mAgents.value( inst->agentType() ).name;
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
        mTracer->error( QLatin1String( "AgentManager::updatePluginInfos" ),
                        QString( "Duplicated agent identifier '%1' from file '%2'" )
                            .arg( fileName, agentInfo.identifier ) );
        continue;
      }
      qDebug() << "PLUGINS inserting: " << agentInfo.identifier << agentInfo.instanceCounter << agentInfo.capabilities;
      mAgents.insert( agentInfo.identifier, agentInfo );
      mAgentWatcher->addPath( Akonadi::XdgBaseDirs::findExecutableFile( agentInfo.exec ) );
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

    AgentInstance::Ptr instance( new AgentInstance( this ) );
    instance->setIdentifier( instanceIdentifier );
    if ( instance->start( mAgents.value( agentType ) ) )
      mAgentInstances.insert( instanceIdentifier, instance );
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

void AgentManager::serviceOwnerChanged( const QString &name, const QString&, const QString &newOwner )
{
  if ( name == "org.freedesktop.Akonadi" && !newOwner.isEmpty() ) {
    // server is operational, start agents
    continueStartup();
  }

  if ( name.startsWith( "org.freedesktop.Akonadi.Agent." ) ) {
    if ( newOwner.isEmpty() )
      return;

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

  else if ( name.startsWith( "org.freedesktop.Akonadi.Resource." ) ) {
    if ( newOwner.isEmpty() )
      return;

    const QString identifier = name.mid( 33 );
    if ( !mAgentInstances.contains( identifier ) )
      return;

    mAgentInstances.value( identifier )->obtainResourceInterface();
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
  if ( mAgentInstances.contains( info.identifier ) )
    return; // already running
  AgentInstance::Ptr instance( new AgentInstance( this ) );
  instance->setIdentifier( info.identifier );
  if ( instance->start( info ) ) {
    mAgentInstances.insert( instance->identifier(), instance );
    save();
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


#include "agentmanager.moc"
