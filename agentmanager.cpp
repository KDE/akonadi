/*
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

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
#include "agentmanager_p.h"

#include "agenttype_p.h"
#include "agentinstance_p.h"

#include "collection.h"

#include <QtGui/QWidget>


using namespace Akonadi;

AgentInstance AgentManagerPrivate::createInstance( const AgentType &type )
{
  const QString &identifier = mManager->createAgentInstance( type.identifier() );
  if ( identifier.isEmpty() )
    return AgentInstance();

  return fillAgentInstanceLight( identifier );
}

void AgentManagerPrivate::agentTypeAdded( const QString &identifier )
{
  const AgentType type = fillAgentType( identifier );
  if ( type.isValid() ) {
    mTypes.insert( identifier, type );
    emit mParent->typeAdded( type );
  }
}

void AgentManagerPrivate::agentTypeRemoved( const QString &identifier )
{
  if ( !mTypes.contains( identifier ) )
    return;

  const AgentType type = mTypes.take( identifier );
  emit mParent->typeRemoved( type );
}

void AgentManagerPrivate::agentInstanceAdded( const QString &identifier )
{
  const AgentInstance instance = fillAgentInstance( identifier );
  if ( instance.isValid() ) {
    mInstances.insert( identifier, instance );
    emit mParent->instanceAdded( instance );
  }
}

void AgentManagerPrivate::agentInstanceRemoved( const QString &identifier )
{
  if ( !mInstances.contains( identifier ) )
    return;

  const AgentInstance instance = mInstances.take( identifier );
  emit mParent->instanceRemoved( instance );
}

void AgentManagerPrivate::agentInstanceStatusChanged( const QString &identifier, int status, const QString &msg )
{
  if ( !mInstances.contains( identifier ) )
    return;

  AgentInstance &instance = mInstances[ identifier ];
  instance.d->mStatus = status;
  instance.d->mStatusMessage = msg;

  emit mParent->instanceStatusChanged( instance );
}

void AgentManagerPrivate::agentInstanceProgressChanged( const QString &identifier, uint progress, const QString &msg )
{
  if ( !mInstances.contains( identifier ) )
    return;

  AgentInstance &instance = mInstances[ identifier ];
  instance.d->mProgress = progress;
  instance.d->mStatusMessage = msg;

  emit mParent->instanceProgressChanged( instance );
}

void AgentManagerPrivate::agentInstanceWarning( const QString &identifier, const QString &msg )
{
  if ( !mInstances.contains( identifier ) )
    return;

  AgentInstance &instance = mInstances[ identifier ];
  emit mParent->instanceWarning( instance, msg );
}

void AgentManagerPrivate::agentInstanceError( const QString &identifier, const QString &msg )
{
  if ( !mInstances.contains( identifier ) )
    return;

  AgentInstance &instance = mInstances[ identifier ];
  emit mParent->instanceError( instance, msg );
}

void AgentManagerPrivate::agentInstanceNameChanged( const QString &identifier, const QString &name )
{
  if ( !mInstances.contains( identifier ) )
    return;

  AgentInstance &instance = mInstances[ identifier ];
  instance.d->mName = name;

  emit mParent->instanceNameChanged( instance );
}

AgentType AgentManagerPrivate::fillAgentType( const QString &identifier ) const
{
  AgentType type;
  type.d->mIdentifier = identifier;
  type.d->mName = mManager->agentName( identifier );
  type.d->mDescription = mManager->agentComment( identifier );
  type.d->mIconName = mManager->agentIcon( identifier );
  type.d->mMimeTypes = mManager->agentMimeTypes( identifier );
  type.d->mCapabilities = mManager->agentCapabilities( identifier );

  return type;
}

void AgentManagerPrivate::setName( const AgentInstance &instance, const QString &name )
{
  mManager->setAgentInstanceName( instance.identifier(), name );
}

void AgentManagerPrivate::setOnline( const AgentInstance &instance, bool state )
{
  mManager->setAgentInstanceOnline( instance.identifier(), state );
}

void AgentManagerPrivate::configure( const AgentInstance &instance, QWidget *parent )
{
  qlonglong winId = 0;
  if ( parent )
    winId = (qlonglong)( parent->window()->winId() );

  mManager->agentInstanceConfigure( instance.identifier(), winId );
}

void AgentManagerPrivate::synchronize( const AgentInstance &instance )
{
  mManager->agentInstanceSynchronize( instance.identifier() );
}

void AgentManagerPrivate::synchronizeCollectionTree( const AgentInstance &instance )
{
  mManager->agentInstanceSynchronizeCollectionTree( instance.identifier() );
}

AgentInstance AgentManagerPrivate::fillAgentInstance( const QString &identifier ) const
{
  AgentInstance instance;

  const QString agentTypeIdentifier = mManager->agentInstanceType( identifier );
  Q_ASSERT_X( mTypes.contains( agentTypeIdentifier ), "fillAgentInstance", "Requests non-existing agent type" );

  instance.d->mType = mTypes.value( agentTypeIdentifier );
  instance.d->mIdentifier = identifier;
  instance.d->mName = mManager->agentInstanceName( identifier );
  instance.d->mStatus = mManager->agentInstanceStatus( identifier );
  instance.d->mStatusMessage = mManager->agentInstanceStatusMessage( identifier );
  instance.d->mProgress = mManager->agentInstanceProgress( identifier );
  instance.d->mIsOnline = mManager->agentInstanceOnline( identifier );

  return instance;
}

AgentInstance AgentManagerPrivate::fillAgentInstanceLight( const QString &identifier ) const
{
  AgentInstance instance;

  const QString agentTypeIdentifier = mManager->agentInstanceType( identifier );
  Q_ASSERT_X( mTypes.contains( agentTypeIdentifier ), "fillAgentInstanceLight", "Requests non-existing agent type" );

  instance.d->mType = mTypes.value( agentTypeIdentifier );
  instance.d->mIdentifier = identifier;

  return instance;
}

AgentManager* AgentManagerPrivate::mSelf = 0;

AgentManager::AgentManager()
  : QObject( 0 ), d( new AgentManagerPrivate( this ) )
{
  d->mManager = new org::freedesktop::Akonadi::AgentManager( QLatin1String( "org.freedesktop.Akonadi.Control" ),
                                                     QLatin1String( "/AgentManager" ),
                                                     QDBusConnection::sessionBus(), this );

  connect( d->mManager, SIGNAL( agentTypeAdded( const QString& ) ),
           this, SLOT( agentTypeAdded( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentTypeRemoved( const QString& ) ),
           this, SLOT( agentTypeRemoved( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceAdded( const QString& ) ),
           this, SLOT( agentInstanceAdded( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceRemoved( const QString& ) ),
           this, SLOT( agentInstanceRemoved( const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceStatusChanged( const QString&, int, const QString& ) ),
           this, SLOT( agentInstanceStatusChanged( const QString&, int, const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceProgressChanged( const QString&, uint, const QString& ) ),
           this, SLOT( agentInstanceProgressChanged( const QString&, uint, const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceNameChanged( const QString&, const QString& ) ),
           this, SLOT( agentInstanceNameChanged( const QString&, const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceWarning( const QString&, const QString& ) ),
           this, SLOT( agentInstanceWarning( const QString&, const QString& ) ) );
  connect( d->mManager, SIGNAL( agentInstanceError( const QString&, const QString& ) ),
           this, SLOT( agentInstanceError( const QString&, const QString& ) ) );

  const QStringList typeIdentifiers = d->mManager->agentTypes();
  foreach( const QString &type, typeIdentifiers ) {
    const AgentType agentType = d->fillAgentType( type );
    d->mTypes.insert( type, agentType );
  }

  const QStringList instanceIdentifiers = d->mManager->agentInstances();
  foreach( const QString &instance, instanceIdentifiers ) {
    const AgentInstance agentInstance = d->fillAgentInstance( instance );
    d->mInstances.insert( instance, agentInstance );
  }
}

AgentManager::~AgentManager()
{
  delete d;
}

AgentManager* AgentManager::self()
{
  if ( !AgentManagerPrivate::mSelf )
    AgentManagerPrivate::mSelf = new AgentManager();

  return AgentManagerPrivate::mSelf;
}

AgentType::List AgentManager::types() const
{
  return d->mTypes.values();
}

AgentType AgentManager::type( const QString &identifier ) const
{
  return d->mTypes.value( identifier );
}

AgentInstance::List AgentManager::instances() const
{
  return d->mInstances.values();
}

AgentInstance AgentManager::instance( const QString &identifier ) const
{
  return d->mInstances.value( identifier );
}

void AgentManager::removeInstance( const AgentInstance &instance )
{
  d->mManager->removeAgentInstance( instance.identifier() );
}

void AgentManager::synchronizeCollection(const Collection & collection)
{
  const QString resId = collection.resource();
  Q_ASSERT( !resId.isEmpty() );
  d->mManager->agentInstanceSynchronizeCollection( resId, collection.id() );
}

#include "agentmanager.moc"
