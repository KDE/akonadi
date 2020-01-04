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
#include "akonadi.h"
#include "tracer.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "resourcemanageradaptor.h"

#include <shared/akranges.h>
#include <private/capabilities_p.h>

#include <QDBusConnection>

using namespace Akonadi::Server;
using namespace AkRanges;

ResourceManager::ResourceManager(AkonadiServer &akonadi)
    : QObject()
    , mAkonadi(akonadi)
{
    new ResourceManagerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/ResourceManager"), this);
}

void ResourceManager::addResourceInstance(const QString &name, const QStringList &capabilities)
{
    Transaction transaction(DataStore::self(), QStringLiteral("ADD RESOURCE INSTANCE"));
    Resource resource = Resource::retrieveByName(name);
    if (resource.isValid()) {
        mAkonadi.tracer().error("ResourceManager", QStringLiteral("Resource '%1' already exists.").arg(name));
        return; // resource already exists
    }

    // create the resource
    resource.setName(name);
    resource.setIsVirtual(capabilities.contains(QLatin1String(AKONADI_AGENT_CAPABILITY_VIRTUAL)));
    if (!resource.insert()) {
        mAkonadi.tracer().error("ResourceManager", QStringLiteral("Could not create resource '%1'.").arg(name));
    }
    transaction.commit();
}

void ResourceManager::removeResourceInstance(const QString &name)
{
    // remove items and collections
    Resource resource = Resource::retrieveByName(name);
    if (resource.isValid()) {
        resource.collections() | Actions::forEach([](Collection col) {
                DataStore::self()->cleanupCollection(col);
            });

        // remove resource
        resource.remove();
    }
}

QStringList ResourceManager::resourceInstances() const
{
    return Resource::retrieveAll() | Views::transform(&Resource::name) | Actions::toQList;
}

