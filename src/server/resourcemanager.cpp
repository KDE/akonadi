/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "resourcemanager.h"
#include "resourcemanageradaptor.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "tracer.h"

#include "private/capabilities_p.h"
#include "shared/akranges.h"

#include <QDBusConnection>

using namespace Akonadi::Server;
using namespace AkRanges;

ResourceManager::ResourceManager(Tracer &tracer)
    : mTracer(tracer)
{
    new ResourceManagerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/ResourceManager"), this);
}

void ResourceManager::addResourceInstance(const QString &name, const QStringList &capabilities)
{
    Transaction transaction(DataStore::self(), QStringLiteral("ADD RESOURCE INSTANCE"));
    Resource resource = Resource::retrieveByName(name);
    if (resource.isValid()) {
        mTracer.error("ResourceManager", QStringLiteral("Resource '%1' already exists.").arg(name));
        return; // resource already exists
    }

    // create the resource
    resource.setName(name);
    resource.setIsVirtual(capabilities.contains(QLatin1StringView(AKONADI_AGENT_CAPABILITY_VIRTUAL)));
    if (!resource.insert()) {
        mTracer.error("ResourceManager", QStringLiteral("Could not create resource '%1'.").arg(name));
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

#include "moc_resourcemanager.cpp"
