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

#include <KDebug>
#include <KGlobal>

#include <QtDBus>

#define AKONADI_CONTROL_SERVICE QLatin1String("org.freedesktop.Akonadi.Control")
#define AKONADI_SERVER_SERVICE QLatin1String("org.freedesktop.Akonadi")

using namespace Akonadi;

class Akonadi::ServerManagerPrivate
{
  public:
    ServerManagerPrivate() : instance( new ServerManager( this ) )
    {
    }

    ~ServerManagerPrivate()
    {
      delete instance;
    }

    void serviceOwnerChanged( const QString &service, const QString &oldOwner, const QString &newOwner )
    {
      if ( service != AKONADI_SERVER_SERVICE )
        return;
      if ( newOwner.isEmpty() && !oldOwner.isEmpty() )
        emit instance->stopped();
      if ( !newOwner.isEmpty() && oldOwner.isEmpty() )
        emit instance->started();
    }

    ServerManager *instance;
};

K_GLOBAL_STATIC( ServerManagerPrivate, sInstance )

ServerManager::ServerManager(ServerManagerPrivate * dd ) :
    d( dd )
{
  connect( QDBusConnection::sessionBus().interface(),
           SIGNAL(serviceOwnerChanged(QString,QString,QString)),
           SLOT(serviceOwnerChanged(QString,QString,QString)) );
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
  return QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_CONTROL_SERVICE )
      && QDBusConnection::sessionBus().interface()->isServiceRegistered( AKONADI_SERVER_SERVICE );
}


#include "servermanager.moc"
