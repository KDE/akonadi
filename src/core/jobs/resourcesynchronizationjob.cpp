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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "resourcesynchronizationjob.h"
#include "KDBusConnectionPool"
#include "kjobprivatebase_p.h"
#include "servermanager.h"
#include "agentinstance.h"
#include "agentmanager.h"

#include <QDebug>
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
        , interface(0)
        , safetyTimer(0)
        , timeoutCount(0)
        , collectionTreeOnly(false)
    {
    }

    void doStart();

    ResourceSynchronizationJob *q;
    AgentInstance instance;
    QDBusInterface *interface;
    QTimer *safetyTimer;
    int timeoutCount;
    bool collectionTreeOnly;
    static const int timeoutCountLimit;

    void slotSynchronized();
    void slotTimeout();
};

const int ResourceSynchronizationJobPrivate::timeoutCountLimit = 60;

ResourceSynchronizationJob::ResourceSynchronizationJob(const AgentInstance &instance, QObject *parent)
    : KJob(parent)
    , d(new ResourceSynchronizationJobPrivate(this))
{
    d->instance = instance;
    d->safetyTimer = new QTimer(this);
    connect(d->safetyTimer, SIGNAL(timeout()), SLOT(slotTimeout()));
    d->safetyTimer->setInterval(10 * 1000);
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
                                   QString::fromLatin1("/"),
                                   QString::fromLatin1("org.freedesktop.Akonadi.Resource"),
                                   KDBusConnectionPool::threadConnection(), this);
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
        qDebug() << "trying again to sync resource" << instance.identifier();
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
