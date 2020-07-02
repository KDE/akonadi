/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvraitl@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "intervalcheck.h"
#include "storage/datastore.h"
#include "storage/itemretrievalmanager.h"
#include "storage/entity.h"

using namespace Akonadi::Server;

static const int MINIMUM_AUTOSYNC_INTERVAL = 5; // minutes
static const int MINIMUM_COLTREESYNC_INTERVAL = 5; // minutes

IntervalCheck::IntervalCheck(ItemRetrievalManager &itemRetrievalManager)
    : CollectionScheduler(QStringLiteral("IntervalCheck"), QThread::IdlePriority)
    , mItemRetrievalManager(itemRetrievalManager)
{
}

IntervalCheck::~IntervalCheck()
{
    quitThread();
}

void IntervalCheck::requestCollectionSync(const Collection &collection)
{
    QMetaObject::invokeMethod(this, [this, collection]() { collectionExpired(collection); }, Qt::QueuedConnection);
}

int IntervalCheck::collectionScheduleInterval(const Collection &collection)
{
    return collection.cachePolicyCheckInterval();
}

bool IntervalCheck::hasChanged(const Collection &collection, const Collection &changed)
{
    return collection.cachePolicyCheckInterval() != changed.cachePolicyCheckInterval()
           || collection.enabled() != changed.enabled()
           || collection.syncPref() != changed.syncPref();
}

bool IntervalCheck::shouldScheduleCollection(const Collection &collection)
{
    return collection.cachePolicyCheckInterval() > 0
           && ((collection.syncPref() == Collection::True) || ((collection.syncPref() == Collection::Undefined) && collection.enabled()));
}

void IntervalCheck::collectionExpired(const Collection &collection)
{
    const QDateTime now(QDateTime::currentDateTime());

    if (collection.parentId() == 0) {
        const QString resourceName = collection.resource().name();

        const int interval = qMax(MINIMUM_COLTREESYNC_INTERVAL, collection.cachePolicyCheckInterval());

        const QDateTime lastExpectedCheck = now.addSecs(interval * -60);
        if (!mLastCollectionTreeSyncs.contains(resourceName) || mLastCollectionTreeSyncs.value(resourceName) < lastExpectedCheck) {
            mLastCollectionTreeSyncs.insert(resourceName, now);
            mItemRetrievalManager.triggerCollectionTreeSync(resourceName);
        }
    }

    // now on to the actual collection syncing
    const int interval = qMax(MINIMUM_AUTOSYNC_INTERVAL, collection.cachePolicyCheckInterval());

    const QDateTime lastExpectedCheck = now.addSecs(interval * -60);
    if (mLastChecks.contains(collection.id()) && mLastChecks.value(collection.id()) > lastExpectedCheck) {
        return;
    }
    mLastChecks.insert(collection.id(), now);
    mItemRetrievalManager.triggerCollectionSync(collection.resource().name(), collection.id());
}
