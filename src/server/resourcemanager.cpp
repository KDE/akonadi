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
#include "storage/transaction.h"
#include "resourcemanageradaptor.h"

#include <private/capabilities_p.h>

#include <QDBusConnection>

using namespace Akonadi::Server;

ResourceManager *ResourceManager::mSelf = 0;

ResourceManager::ResourceManager(QObject *parent)
    : QObject(parent)
{
    new ResourceManagerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/ResourceManager"), this);
}

void ResourceManager::addResourceInstance(const QString &name, const QStringList &capabilities)
{
    Transaction transaction(DataStore::self());
    Resource resource = Resource::retrieveByName(name);
    if (resource.isValid()) {
        Tracer::self()->error("ResourceManager", QStringLiteral("Resource '%1' already exists.").arg(name));
        return; // resource already exists
    }

    // create the resource
    resource.setName(name);
    resource.setIsVirtual(capabilities.contains(QStringLiteral(AKONADI_AGENT_CAPABILITY_VIRTUAL)));
    if (!resource.insert()) {
        Tracer::self()->error("ResourceManager", QStringLiteral("Could not create resource '%1'.").arg(name));
    }
    transaction.commit();
}

void ResourceManager::removeResourceInstance(const QString &name)
{
    DataStore *db = DataStore::self();

    // remove items and collections
    Resource resource = Resource::retrieveByName(name);
    if (resource.isValid()) {
        const QVector<Collection> collections = resource.collections();
        Q_FOREACH (/*sic!*/ Collection collection, collections) { // krazy:exclude=foreach
            db->cleanupCollection(collection);
        }

        // remove resource
        resource.remove();
    }
}

QStringList ResourceManager::resourceInstances() const
{
    QStringList result;
    const auto resources = Resource::retrieveAll();
    result.reserve(resources.size());
    for (const Resource &res : resources) {
        result.append(res.name());
    }
    return result;
}

ResourceManager *ResourceManager::self()
{
    if (!mSelf) {
        mSelf = new ResourceManager();
    }
    return mSelf;
}
