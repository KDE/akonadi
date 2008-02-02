/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include "agentmanager.h"

#include "agentmanagerinterface.h"

#include <kicon.h>
#include <kiconloader.h>

#include <QtGui/QIcon>


using namespace Akonadi;

class AgentManager::Private
{
  public:
    Private( AgentManager *parent )
      : mParent( parent )
    {
    }

    void agentInstanceStatusChanged( const QString &agent, int state, const QString &message )
    {
      Status status = Ready;
      switch ( state ) {
        case 0:
        default:
          status = Ready;
          break;
        case 1:
          status = Syncing;
          break;
        case 2:
          status = Error;
          break;
      }

      emit mParent->agentInstanceStatusChanged( agent, status, message );
    }

    AgentManager *mParent;
    org::kde::Akonadi::AgentManager *mManager;
};

AgentManager::AgentManager( QObject *parent )
  : QObject( parent ), d( new Private( this ) )
{
  d->mManager = new org::kde::Akonadi::AgentManager( QLatin1String( "org.kde.Akonadi.Control" ),
                                                     QLatin1String( "/AgentManager" ),
                                                     QDBusConnection::sessionBus(), this );

  connect( d->mManager, SIGNAL( agentTypeAdded( const QString& ) ),
           this, SIGNAL( agentTypeAdded( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentTypeRemoved( const QString& ) ),
           this, SIGNAL( agentTypeRemoved( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceAdded( const QString& ) ),
           this, SIGNAL( agentInstanceAdded( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceRemoved( const QString& ) ),
           this, SIGNAL( agentInstanceRemoved( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceStatusChanged( const QString&, int, const QString& ) ),
           this, SLOT( agentInstanceStatusChanged( const QString&, int, const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceProgressChanged( const QString&, uint, const QString& ) ),
           this, SIGNAL( agentInstanceProgressChanged( const QString&, uint, const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceNameChanged( const QString&, const QString& ) ),
           this, SIGNAL( agentInstanceNameChanged( const QString&, const QString& ) ) );
}

AgentManager::~AgentManager()
{
  delete d;
}

QStringList AgentManager::agentTypes() const
{
  return d->mManager->agentTypes();
}

QString AgentManager::agentName( const QString &identifier ) const
{
  return d->mManager->agentName( identifier );
}

QString AgentManager::agentComment( const QString &identifier ) const
{
  return d->mManager->agentComment( identifier );
}

QString AgentManager::agentIconName( const QString &identifier ) const
{
  return d->mManager->agentIcon( identifier );
}

QIcon AgentManager::agentIcon( const QString &identifier ) const
{
  return KIcon( agentIconName( identifier ), KIconLoader::global() );
}

QStringList AgentManager::agentMimeTypes( const QString &identifier ) const
{
  return d->mManager->agentMimeTypes( identifier );
}

QStringList AgentManager::agentCapabilities( const QString &identifier ) const
{
  return d->mManager->agentCapabilities( identifier );
}

QString AgentManager::createAgentInstance( const QString &identifier )
{
  return d->mManager->createAgentInstance( identifier );
}

void AgentManager::removeAgentInstance( const QString &identifier )
{
  d->mManager->removeAgentInstance( identifier );
}

QString AgentManager::agentInstanceType( const QString &identifier )
{
  return d->mManager->agentInstanceType( identifier );
}

QStringList AgentManager::agentInstances() const
{
  return d->mManager->agentInstances();
}

AgentManager::Status AgentManager::agentInstanceStatus( const QString &identifier ) const
{
  int status = d->mManager->agentInstanceStatus( identifier );
  switch ( status ) {
    case 0:
    default:
      return Ready;
      break;
    case 1:
      return Syncing;
      break;
    case 2:
      return Error;
      break;
  }
}

QString AgentManager::agentInstanceStatusMessage( const QString &identifier ) const
{
  return d->mManager->agentInstanceStatusMessage( identifier );
}

uint AgentManager::agentInstanceProgress( const QString &identifier ) const
{
  return d->mManager->agentInstanceProgress( identifier );
}

QString AgentManager::agentInstanceProgressMessage( const QString &identifier ) const
{
  return d->mManager->agentInstanceProgressMessage( identifier );
}

void AgentManager::setAgentInstanceName( const QString &identifier, const QString &name )
{
  d->mManager->setAgentInstanceName( identifier, name );
}

QString AgentManager::agentInstanceName( const QString &identifier ) const
{
  return d->mManager->agentInstanceName( identifier );
}

void AgentManager::agentInstanceConfigure( const QString &identifier )
{
  d->mManager->agentInstanceConfigure( identifier );
}

void AgentManager::agentInstanceSynchronize( const QString &identifier )
{
  d->mManager->agentInstanceSynchronize( identifier );
}

bool AgentManager::agentInstanceOnline(const QString & identifier)
{
  return d->mManager->agentInstanceOnline( identifier );
}

void AgentManager::setAgentInstanceOnline(const QString & identifier, bool state)
{
  d->mManager->setAgentInstanceOnline( identifier, state );
}

void AgentManager::agentInstanceSynchronizeCollectionTree(const QString & identifier)
{
  d->mManager->agentInstanceSynchronizeCollectionTree( identifier );
}

void AgentManager::agentInstanceSynchronizeCollection(const Collection & collection)
{
  const QString resId = collection.resource();
  Q_ASSERT( !resId.isEmpty() );
  d->mManager->agentInstanceSynchronizeCollection( resId, collection.id() );
}

#include "agentmanager.moc"
