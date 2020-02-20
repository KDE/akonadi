/*
 * Copyright (c) 2009 Volker Krause <vkrause@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "resourcesynchronizationjob.h"
#include <QDBusConnection>
#include "kjobprivatebase_p.h"
#include "servermanager.h"
#include "agentinstance.h"
#include "agentmanager.h"
#include "akonadicore_debug.h"

#include <KLocalizedString>

#include <QDBusInterface>
#include <QTimer>

namespace Akonadi
{

class ResourceSynchronizationJobPrivate : public KJobPrivateBase
{
public:
    ResourceSynchronizationJobPrivate(ResourceSynchronizationJob *parent)
        : q(parent)
    {
    }

    void doStart() override;

    ResourceSynchronizationJob *q;
    AgentInstance instance;
    QDBusInterface *interface = nullptr;
    QTimer *safetyTimer = nullptr;
    int timeoutCount = 60;
    bool collectionTreeOnly = false;
    int timeoutCountLimit = 0;

    void slotSynchronized();
    void slotTimeout();
};

ResourceSynchronizationJob::ResourceSynchronizationJob(const AgentInstance &instance, QObject *parent)
    : KJob(parent)
    , d(new ResourceSynchronizationJobPrivate(this))
{
    d->instance = instance;
    d->safetyTimer = new QTimer(this);
    connect(d->safetyTimer, &QTimer::timeout, this, [this]() { d->slotTimeout(); });
    d->safetyTimer->setInterval(30 * 1000);
    d->safetyTimer->setSingleShot(false);
}

ResourceSynchronizationJob::~ResourceSynchronizationJob()
{
    delete d;
}

void ResourceSynchronizationJob::start()
{
    d->start();
}

void ResourceSynchronizationJob::setTimeoutCountLimit(int count)
{
    d->timeoutCountLimit = count;
}

int ResourceSynchronizationJob::timeoutCountLimit() const
{
    return d->timeoutCountLimit;
}

bool ResourceSynchronizationJob::collectionTreeOnly() const
{
    return d->collectionTreeOnly;
}

void ResourceSynchronizationJob::setCollectionTreeOnly(bool b)
{
    d->collectionTreeOnly = b;
}

void ResourceSynchronizationJobPrivate::doStart()
{
    if (!instance.isValid()) {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Invalid resource instance."));
        q->emitResult();
        return;
    }

    interface = new QDBusInterface(ServerManager::agentServiceName(ServerManager::Resource, instance.identifier()),
                                   QStringLiteral("/"),
                                   QStringLiteral("org.freedesktop.Akonadi.Resource"),
                                   QDBusConnection::sessionBus(), this);
    if (collectionTreeOnly) {
        connect(interface, SIGNAL(collectionTreeSynchronized()), q, SLOT(slotSynchronized()));
    } else {
        connect(interface, SIGNAL(synchronized()), q, SLOT(slotSynchronized()));
    }

    if (interface->isValid()) {
        if (collectionTreeOnly) {
            instance.synchronizeCollectionTree();
        } else {
            instance.synchronize();
        }

        safetyTimer->start();
    } else {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Unable to obtain D-Bus interface for resource '%1'", instance.identifier()));
        q->emitResult();
        return;
    }
}

void ResourceSynchronizationJobPrivate::slotSynchronized()
{
    if (collectionTreeOnly) {
        q->disconnect(interface, SIGNAL(collectionTreeSynchronized()), q, SLOT(slotSynchronized()));
    } else {
        q->disconnect(interface, SIGNAL(synchronized()), q, SLOT(slotSynchronized()));
    }
    safetyTimer->stop();
    q->emitResult();
}

void ResourceSynchronizationJobPrivate::slotTimeout()
{
    instance = AgentManager::self()->instance(instance.identifier());
    timeoutCount++;

    if (timeoutCount > timeoutCountLimit) {
        safetyTimer->stop();
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Resource synchronization timed out."));
        q->emitResult();
        return;
    }

    if (instance.status() == AgentInstance::Idle) {
        // try again, we might have lost the synchronized()/synchronizedCollectionTree() signal
        qCDebug(AKONADICORE_LOG) << "trying again to sync resource" << instance.identifier();
        if (collectionTreeOnly) {
            instance.synchronizeCollectionTree();
        } else {
            instance.synchronize();
        }
    }
}

AgentInstance ResourceSynchronizationJob::resource() const
{
    return d->instance;
}

}

#include "moc_resourcesynchronizationjob.cpp"
