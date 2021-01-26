/*
 * SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "resourcesynchronizationjob.h"
#include "agentinstance.h"
#include "agentmanager.h"
#include "akonadicore_debug.h"
#include "kjobprivatebase_p.h"
#include "resourceinterface.h"
#include "servermanager.h"
#include <QDBusConnection>

#include <KLocalizedString>

#include <QDBusInterface>
#include <QTimer>

namespace Akonadi
{
class ResourceSynchronizationJobPrivate : public KJobPrivateBase
{
    Q_OBJECT

public:
    explicit ResourceSynchronizationJobPrivate(ResourceSynchronizationJob *parent)
        : q(parent)
    {
        connect(&safetyTimer, &QTimer::timeout, this, &ResourceSynchronizationJobPrivate::slotTimeout);
        safetyTimer.setInterval(std::chrono::seconds{30});
        safetyTimer.setSingleShot(true);
    }

    void doStart() override;

    ResourceSynchronizationJob *const q;
    AgentInstance instance;
    std::unique_ptr<org::freedesktop::Akonadi::Resource> interface;
    QTimer safetyTimer;
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

    using ResourceIface = org::freedesktop::Akonadi::Resource;
    interface = std::make_unique<ResourceIface>(ServerManager::agentServiceName(ServerManager::Resource, instance.identifier()),
                                                QStringLiteral("/"),
                                                QDBusConnection::sessionBus());
    if (collectionTreeOnly) {
        connect(interface.get(), &ResourceIface::collectionTreeSynchronized, this, &ResourceSynchronizationJobPrivate::slotSynchronized);
    } else {
        connect(interface.get(), &ResourceIface::synchronized, this, &ResourceSynchronizationJobPrivate::slotSynchronized);
    }

    if (interface->isValid()) {
        if (collectionTreeOnly) {
            instance.synchronizeCollectionTree();
        } else {
            instance.synchronize();
        }

        safetyTimer.start();
    } else {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Unable to obtain D-Bus interface for resource '%1'", instance.identifier()));
        q->emitResult();
        return;
    }
}

void ResourceSynchronizationJobPrivate::slotSynchronized()
{
    using ResourceIface = org::freedesktop::Akonadi::Resource;
    if (collectionTreeOnly) {
        disconnect(interface.get(), &ResourceIface::collectionTreeSynchronized, this, &ResourceSynchronizationJobPrivate::slotSynchronized);
    } else {
        disconnect(interface.get(), &ResourceIface::synchronized, this, &ResourceSynchronizationJobPrivate::slotSynchronized);
    }
    safetyTimer.stop();
    q->emitResult();
}

void ResourceSynchronizationJobPrivate::slotTimeout()
{
    instance = AgentManager::self()->instance(instance.identifier());
    timeoutCount++;

    if (timeoutCount > timeoutCountLimit) {
        safetyTimer.stop();
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

} // namespace Akonadi

#include "resourcesynchronizationjob.moc"
