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
#include "agentmanager.h"
#include "session_p.h"

#include <KDebug>
#include <KGlobal>

#include <QtDBus>

#define AKONADI_CONTROL_SERVICE QLatin1String("org.freedesktop.Akonadi.Control")
#define AKONADI_SERVER_SERVICE QLatin1String("org.freedesktop.Akonadi")

using namespace Akonadi;

class Akonadi::ServerManagerPrivate
{
  public:
    ServerManagerPrivate() :
      instance( new ServerManager( this ) ),
      serverProtocolVersion( -1 )
    {
      operational = instance->isRunning();
    }

    ~ServerManagerPrivate()
    {
      delete instance;
    }

    void serviceOwnerChanged( const QString &service, const QString &oldOwner, const QString &newOwner )
    {
      Q_UNUSED( oldOwner );
      Q_UNUSED( newOwner );
      if ( service != AKONADI_SERVER_SERVICE && service != AKONADI_CONTROL_SERVICE )
        return;
      serverProtocolVersion = -1,
      checkStatusChanged();
    }

    void checkStatusChanged()
    {
      const bool status = instance->isRunning();
      if ( status == operational )
        return;
      operational = status;
      if ( operational )
        emit instance->started();
      else
        emit instance->stopped();
    }

    ServerManager *instance;
    int serverProtocolVersion;
    bool operational;
};

K_GLOBAL_STATIC( ServerManagerPrivate, sInstance )

ServerManager::ServerManager(ServerManagerPrivate * dd ) :
    d( dd )
{
  connect( QDBusConnection::sessionBus().interface(),
           SIGNAL(serviceOwnerChanged(QString,QString,QString)),
           SLOT(serviceOwnerChanged(QString,QString,QString)) );
  connect( AgentManager::self(), SIGNAL(typeAdded(Akonadi::AgentType)), SLOT(checkStatusChanged()) );
  connect( AgentManager::self(), SIGNAL(typeRemoved(Akonadi::AgentType)), SLOT(checkStatusChanged()) );
}

ServerManager * Akonadi::ServerManager::instance()
{
  return sInstance->instance;
}

bool ServerManager::start()
{
  QDBusReply<void> reply = QDBusConnection::sessionBus().interface()->startService( AKONADI_CONTROL_SERVICE );
  if ( !reply.isValid() ) {
    kDebug( 5250 ) << "Unable to start Akonadi control process: "
                     << reply.error().message();

    // start via D-Bus .service file didn't work, let's try starting the process manually
    if ( reply.error().type() == QDBusError::ServiceUnknown ) {
      const bool ok = QProcess::startDetached( QLatin1String("akonadi_control") );
      if ( !ok ) {
        kWarning( 5250 ) << "Error: unable to execute binary akonadi_control";
        return false;
      }
    } else {
      return false;
    }
  }

  return true;
}

bool ServerManager::stop()
{
  QDBusInterface iface( AKONADI_CONTROL_SERVICE, "/ControlManager", "org.freedesktop.Akonadi.ControlManager" );
  if ( !iface.isValid() )
    return false;
  iface.call( QDBus::NoBlock, "shutdown" );
  return true;
}

bool ServerManager::isRunning()
{
  if ( !QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE ) ||
       !QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_SERVER_SERVICE ) ) {
    return false;
  }

  // check if the server protocol is recent enough
  if ( sInstance.exists() ) {
    if ( sInstance->serverProtocolVersion >= 0 && sInstance->serverProtocolVersion < SessionPrivate::minimumProtocolVersion() )
      return false;
  }

  // besides the running server processes we also need at least one resource to be operational
  AgentType::List agentTypes = AgentManager::self()->types();
  foreach ( const AgentType &type, agentTypes ) {
    if ( type.capabilities().contains( "Resource" ) )
      return true;
  }
  return false;
}

int Internal::serverProtocolVersion()
{
  return sInstance->serverProtocolVersion;
}

void Internal::setServerProtocolVersion( int version )
{
  sInstance->serverProtocolVersion = version;
  sInstance->checkStatusChanged();
}

#include "servermanager.moc"
