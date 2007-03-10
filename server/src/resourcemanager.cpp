/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "resourcemanager.h"
#include "tracer.h"
#include "storage/datastore.h"

#include <QtDBus/QDBusConnection>

using namespace Akonadi;

ResourceManager* ResourceManager::mSelf = 0;

Akonadi::ResourceManager::ResourceManager(QObject * parent) :
  QObject( parent )
{
  mManager = new org::kde::Akonadi::AgentManager( QLatin1String("org.kde.Akonadi.Control"),
      QLatin1String("/AgentManager"), QDBusConnection::sessionBus(), this );

  connect( mManager, SIGNAL(agentInstanceAdded(const QString&)),
           SLOT(resourceAdded(const QString&)) );
  connect( mManager, SIGNAL(agentInstanceRemoved(const QString&)),
           SLOT(resourceRemoved(const QString& )) );
}

void Akonadi::ResourceManager::resourceAdded(const QString & name)
{
  DataStore *db = DataStore::self();

  Resource resource = Resource::retrieveByName( name );
  if ( resource.isValid() )
    return; // resource already exists

  // create the resource
  if ( !db->appendResource( name ) ) {
    Tracer::self()->error( "ResourceManager", QString::fromLatin1("Could not create resource '%1'.").arg(name) );
    delete db;
    return;
  }
  resource = Resource::retrieveByName( name );

  // create the toplevel collection
  QString collectionName = mManager->agentName( mManager->agentInstanceType( name ) );
  if ( collectionName.isEmpty() )
    collectionName = name;
  Location loc;
  loc.setName( collectionName );
  loc.setParentId( 0 ); // root
  loc.setResourceId( resource.id() );
  if ( !db->appendLocation( loc ) ) {
    loc.setName( name ); // name already in use, try again
    db->appendLocation( loc );
  }
}

void Akonadi::ResourceManager::resourceRemoved(const QString & name)
{
  DataStore *db = DataStore::self();

  // remove items and collections
  Resource resource = Resource::retrieveByName( name );
  if ( resource.isValid() ) {
    QList<Location> locations = resource.locations();
    foreach ( Location location, locations )
      db->cleanupLocation( location );

    // remove resource
    db->removeResource( resource );
  }
}

ResourceManager * Akonadi::ResourceManager::self()
{
  if ( !mSelf )
    mSelf = new ResourceManager();
  return mSelf;
}

#include "resourcemanager.moc"
