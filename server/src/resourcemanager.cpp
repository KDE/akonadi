/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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
#include "resourcemanageradaptor.h"

#include <QtDBus/QDBusConnection>

using namespace Akonadi;

ResourceManager* ResourceManager::mSelf = 0;

Akonadi::ResourceManager::ResourceManager(QObject * parent) :
  QObject( parent )
{
  new ResourceManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String("/ResourceManager"), this );
}

bool Akonadi::ResourceManager::addResourceInstance(const QString & name)
{
  DataStore *db = DataStore::self();

  Resource resource = Resource::retrieveByName( name );
  if ( resource.isValid() )
    return false; // resource already exists

  // create the resource
  resource.setName( name );
  if ( !resource.insert() ) {
    Tracer::self()->error( "ResourceManager", QString::fromLatin1("Could not create resource '%1'.").arg(name) );
    delete db;
    return false;
  }
  resource = Resource::retrieveByName( name );
  return true;
}

bool Akonadi::ResourceManager::removeResourceInstance(const QString & name)
{
  DataStore *db = DataStore::self();

  // remove items and collections
  Resource resource = Resource::retrieveByName( name );
  if ( resource.isValid() ) {
    QList<Collection> collections = resource.collections();
    foreach ( Collection collection, collections )
      db->cleanupCollection( collection );

    // remove resource
    resource.remove();
  }
  return true;
}

ResourceManager * Akonadi::ResourceManager::self()
{
  if ( !mSelf )
    mSelf = new ResourceManager();
  return mSelf;
}

#include "resourcemanager.moc"
