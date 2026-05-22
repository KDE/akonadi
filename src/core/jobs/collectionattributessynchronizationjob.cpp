/*
 * SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "collectionattributessynchronizationjob.h"
#include "akonadicore_debug.h"
#include "kjobprivatebase_p.h"
#include "resourceinterface.h"
#include "servermanager.h"

#include "agentinstance.h"
#include "agentmanager.h"
#include "collection.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QTimer>

namespace Akonadi
{
class CollectionAttributesSynchronizationJobPrivate : public KJobPrivateBase
{
    Q_OBJECT

public:
    explicit CollectionAttributesSynchronizationJobPrivate(CollectionAttributesSynchronizationJob *parent)
        : q(parent)
    {
        connect(&safetyTimer, &QTimer::timeout, this, &CollectionAttributesSynchronizationJobPrivate::slotTimeout);
        safetyTimer.setInterval(std::chrono::seconds{5});
        safetyTimer.setSingleShot(false);
    }

    void doStart() override;

    CollectionAttributesSynchronizationJob *const q;
    AgentInstance instance;
    Collection collection;
    OrgFreedesktopAkonadiResourceInterface *interface = nullptr;
    QTimer safetyTimer;
    int timeoutCount = 0;
    static const int timeoutCountLimit;

private Q_SLOTS:
    void slotSynchronized(qlonglong /*id*/);
    void slotTimeout();
};

const int CollectionAttributesSynchronizationJobPrivate::timeoutCountLimit = 2;

CollectionAttributesSynchronizationJob::CollectionAttributesSynchronizationJob(const Collection &collection, QObject *parent)
    : KJob(parent)
    , d(new CollectionAttributesSynchronizationJobPrivate(this))
{
    d->instance = AgentManager::self()->instance(collection.resource());
    d->collection = collection;
}

CollectionAttributesSynchronizationJob::~CollectionAttributesSynchronizationJob() = default;

void CollectionAttributesSynchronizationJob::start()
{
    d->start();
}

void CollectionAttributesSynchronizationJobPrivate::doStart()
{
    if (!collection.isValid()) {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Invalid collection instance."));
        q->emitResult();
        return;
    }

    if (!instance.isValid()) {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Invalid resource instance."));
        q->emitResult();
        return;
    }

    interface = new OrgFreedesktopAkonadiResourceInterface(ServerManager::agentServiceName(ServerManager::Resource, instance.identifier()),
                                                           QStringLiteral("/"),
                                                           QDBusConnection::sessionBus(),
                                                           this);
    connect(
        interface,
        &OrgFreedesktopAkonadiResourceInterface::attributesSynchronized,
        this,
        [this](qlonglong id) {
            slotSynchronized(id);
        },
        Qt::SingleShotConnection);

    if (interface->isValid()) {
        interface->synchronizeCollectionAttributes(collection.id());
        safetyTimer.start();
    } else {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Unable to obtain D-Bus interface for resource '%1'", instance.identifier()));
        q->emitResult();
        return;
    }
}

void CollectionAttributesSynchronizationJobPrivate::slotSynchronized(qlonglong id)
{
    if (id == collection.id()) {
        safetyTimer.stop();
        q->emitResult();
    }
}

void CollectionAttributesSynchronizationJobPrivate::slotTimeout()
{
    instance = AgentManager::self()->instance(instance.identifier());
    timeoutCount++;

    if (timeoutCount > timeoutCountLimit) {
        safetyTimer.stop();
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Collection attributes synchronization timed out."));
        q->emitResult();
        return;
    }

    if (instance.status() == AgentInstance::Idle) {
        // try again, we might have lost the synchronized() signal
        qCDebug(AKONADICORE_LOG) << "collection attributes" << collection.id() << instance.identifier();
        interface->call(QStringLiteral("synchronizeCollectionAttributes"), collection.id());
    }
}

} // namespace Akonadi

#include "collectionattributessynchronizationjob.moc"

#include "moc_collectionattributessynchronizationjob.cpp"
