/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "servermanager.h"
#include "servermanager_p.h"

#include "agenttype.h"
#include "agentbase.h"
#include "agentmanager.h"
#include "selftestdialog_p.h"
#include "session_p.h"

#include <KDebug>
#include <KGlobal>

#include <QtDBus>
#include <QTimer>

#include <boost/scoped_ptr.hpp>

#define AKONADI_CONTROL_SERVICE QLatin1String( "org.freedesktop.Akonadi.Control" )
#define AKONADI_SERVER_SERVICE QLatin1String( "org.freedesktop.Akonadi" )

using namespace Akonadi;

class Akonadi::ServerManagerPrivate
{
  public:
    ServerManagerPrivate() :
      instance( new ServerManager( this ) ), mState( ServerManager::NotRunning ), mSafetyTimer( new QTimer )
    {
      mState = instance->state();
      mSafetyTimer->setSingleShot( true );
      mSafetyTimer->setInterval( 30000 );
      QObject::connect( mSafetyTimer.get(), SIGNAL( timeout() ), instance, SLOT( timeout() ) );
    }

    ~ServerManagerPrivate()
    {
      delete instance;
    }

    void serviceOwnerChanged( const QString&, const QString&, const QString& )
    {
      serverProtocolVersion = -1,
      checkStatusChanged();
    }

    void checkStatusChanged()
    {
      setState( instance->state() );
    }

    void setState( ServerManager::State state )
    {

      if ( mState != state ) {
        mState = state;
        emit instance->stateChanged( state );
        if ( state == ServerManager::Running )
          emit instance->started();
        else if ( state == ServerManager::NotRunning || state == ServerManager::Broken )
          emit instance->stopped();

        if ( state == ServerManager::Starting || state == ServerManager::Stopping )
          mSafetyTimer->start();
        else
          mSafetyTimer->stop();
      }
    }

    void timeout()
    {
      if ( mState == ServerManager::Starting || mState == ServerManager::Stopping )
        setState( ServerManager::Broken );
    }

    ServerManager *instance;
    static int serverProtocolVersion;
    ServerManager::State mState;
    boost::scoped_ptr<QTimer> mSafetyTimer;
};

int ServerManagerPrivate::serverProtocolVersion = -1;

K_GLOBAL_STATIC( ServerManagerPrivate, sInstance )

ServerManager::ServerManager(ServerManagerPrivate * dd ) :
    d( dd )
{
  QDBusServiceWatcher *watcher = new QDBusServiceWatcher( AKONADI_SERVER_SERVICE,
                                                          QDBusConnection::sessionBus(),
                                                          QDBusServiceWatcher::WatchForOwnerChange, this );
  watcher->addWatchedService( AKONADI_CONTROL_SERVICE );

  connect( watcher, SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
           this, SLOT( serviceOwnerChanged( const QString&, const QString&, const QString& ) ) );

  // HACK see if we are a agent ourselves and skip AgentManager creation since that can cause deadlocks
  QObject *obj = QDBusConnection::sessionBus().objectRegisteredAt( QLatin1String( "/" ) );
  if ( obj && dynamic_cast<AgentBase*>( obj ) )
    return;
  connect( AgentManager::self(), SIGNAL( typeAdded( const Akonadi::AgentType& ) ), SLOT( checkStatusChanged() ) );
  connect( AgentManager::self(), SIGNAL( typeRemoved( const Akonadi::AgentType& ) ), SLOT( checkStatusChanged() ) );
}

ServerManager * Akonadi::ServerManager::self()
{
  return sInstance->instance;
}

bool ServerManager::start()
{
  const bool controlRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE );
  const bool serverRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_SERVER_SERVICE );
  if (  controlRegistered && serverRegistered )
    return true;

  // TODO: use AKONADI_CONTROL_SERVICE_LOCK instead once we depend on a recent enough Akonadi server
  const bool controlLockRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE + QLatin1String( ".lock" ) );
  if ( controlLockRegistered || controlRegistered ) {
    kDebug() << "Akonadi server is already starting up";
    sInstance->setState( Starting );
    return true;
  }

  kDebug() << "executing akonadi_control";
  const bool ok = QProcess::startDetached( QLatin1String( "akonadi_control" ) );
  if ( !ok ) {
    kWarning() << "Unable to execute akonadi_control, falling back to D-Bus auto-launch";
    QDBusReply<void> reply = QDBusConnection::sessionBus().interface()->startService( AKONADI_CONTROL_SERVICE );
    if ( !reply.isValid() ) {
      kDebug() << "Akonadi server could not be started via D-Bus either: "
               << reply.error().message();
      return false;
    }
  }
  sInstance->setState( Starting );
  return true;
}

bool ServerManager::stop()
{
  QDBusInterface iface( AKONADI_CONTROL_SERVICE,
                        QString::fromLatin1( "/ControlManager" ),
                        QString::fromLatin1( "org.freedesktop.Akonadi.ControlManager" ) );
  if ( !iface.isValid() )
    return false;
  iface.call( QDBus::NoBlock, QString::fromLatin1( "shutdown" ) );
  sInstance->setState( Stopping );
  return true;
}

void ServerManager::showSelfTestDialog( QWidget *parent )
{
  Akonadi::SelfTestDialog dlg( parent );
  dlg.hideIntroduction();
  dlg.exec();
}

bool ServerManager::isRunning()
{
  return state() == Running;
}

ServerManager::State ServerManager::state()
{
  ServerManager::State previousState = NotRunning;
  if ( sInstance.exists() ) // be careful, this is called from the ServerManager::Private ctor, so using sInstance unprotected can cause infinite recursion
    previousState = sInstance->mState;

  const bool controlRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE );
  const bool serverRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_SERVER_SERVICE );
  if (  controlRegistered && serverRegistered ) {
    // check if the server protocol is recent enough
    if ( sInstance.exists() ) {
      if ( Internal::serverProtocolVersion() >= 0 &&
           Internal::serverProtocolVersion() < SessionPrivate::minimumProtocolVersion() )
        return Broken;
    }

    // HACK see if we are a agent ourselves and skip the test below which can in some cases deadlock the server
    // and is not really needed in this case anyway since we happen to know at least one agent is available
    QObject *obj = QDBusConnection::sessionBus().objectRegisteredAt( QLatin1String( "/" ) );
    if ( obj && dynamic_cast<AgentBase*>( obj ) )
      return Running;

    // besides the running server processes we also need at least one resource to be operational
    AgentType::List agentTypes = AgentManager::self()->types();
    foreach ( const AgentType &type, agentTypes ) {
      if ( type.capabilities().contains( QLatin1String( "Resource" ) ) )
        return Running;
    }
    return Broken;
  }

  // TODO: use AKONADI_CONTROL_SERVICE_LOCK instead once we depend on a recent enough Akonadi server
  const bool controlLockRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE + QLatin1String( ".lock" ) );
  if ( controlLockRegistered || controlRegistered ) {
    kDebug() << "Akonadi server is already starting up";
    if ( previousState == Running )
      return NotRunning; // we don't know if it's starting or stopping, probably triggered by someone else
    return previousState;
  }

  if ( serverRegistered ) {
    kWarning() << "Akonadi server running without control process!";
    return Broken;
  }

  if ( previousState == Starting || previousState == Broken ) // valid cases where nothing might be running (yet)
    return previousState;
  return NotRunning;
}

int Internal::serverProtocolVersion()
{
  return ServerManagerPrivate::serverProtocolVersion;
}

void Internal::setServerProtocolVersion( int version )
{
  ServerManagerPrivate::serverProtocolVersion = version;
  if ( sInstance.exists() )
    sInstance->checkStatusChanged();
}

#include "servermanager.moc"
