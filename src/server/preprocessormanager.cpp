/******************************************************************************
 *
 *  File : preprocessormanager.cpp
 *  Creation date : Sat 18 Jul 2009 01:58:50
 *
 *  Copyright (c) 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA, 02110-1301, USA.
 *
 *****************************************************************************/

#include "preprocessormanager.h"
#include "akonadiserver_debug.h"
#include "akonadiserver_debug.h"

#include "entities.h" // Akonadi::Server::PimItem
#include "storage/datastore.h"
#include "tracer.h"
#include "utils.h"
#include "collectionreferencemanager.h"

#include "preprocessormanageradaptor.h"

namespace Akonadi
{
namespace Server
{

const int gHeartbeatTimeoutInMSecs = 30000; // 30 sec heartbeat

// 2 minutes should be really enough to process an item.
// After this timeout elapses we assume that the preprocessor
// is "stuck" and we attempt to kick it by requesting an abort().
const int gWarningItemProcessingTimeInSecs = 120;
// After 3 minutes, if the preprocessor is still "stuck" then
// we attempt to restart it via AgentManager....
const int gMaximumItemProcessingTimeInSecs = 180;
// After 4 minutes, if the preprocessor is still "stuck" then
// we assume it's dead and just drop it's interface.
const int gDeadlineItemProcessingTimeInSecs = 240;

} // namespace Server
} // namespace Akonadi

using namespace Akonadi::Server;

// The one and only PreprocessorManager object
PreprocessorManager *PreprocessorManager::mSelf = nullptr;

PreprocessorManager::PreprocessorManager()
    : QObject()
    , mEnabled(true)
    , mMutex(new QMutex())
{
    mSelf = this; // just to have it set early
    // Hook in our D-Bus interface "shell".
    new PreprocessorManagerAdaptor(this);

    QDBusConnection::sessionBus().registerObject(
        QStringLiteral("/PreprocessorManager"),
        this,
        QDBusConnection::ExportAdaptors);

    mHeartbeatTimer = new QTimer(this);

    QObject::connect(mHeartbeatTimer, &QTimer::timeout, this, &PreprocessorManager::heartbeat);

    mHeartbeatTimer->start(gHeartbeatTimeoutInMSecs);
}

PreprocessorManager::~PreprocessorManager()
{
    mHeartbeatTimer->stop();

    // FIXME: Explicitly interrupt pre-processing here ?
    //        Pre-Processors should auto-protect themselves from re-processing an item:
    //        they are "closer to the DB" from this point of view.

    qDeleteAll(mPreprocessorChain);
    qDeleteAll(mTransactionWaitQueueHash);   // this should also disconnect all the signals from the data store objects...

    delete mMutex;
}

bool PreprocessorManager::init()
{
    if (mSelf) {
        return false;
    }
    mSelf = new PreprocessorManager();
    return true;
}

void PreprocessorManager::done()
{
    delete mSelf;
    mSelf = nullptr;
}

bool PreprocessorManager::isActive()
{
    QMutexLocker locker(mMutex);

    if (!mEnabled) {
        return false;
    }
    return mPreprocessorChain.count() > 0;
}

PreprocessorInstance *PreprocessorManager::lockedFindInstance(const QString &id)
{
    for (PreprocessorInstance *instance : qAsConst(mPreprocessorChain)) {
        if (instance->id() == id) {
            return instance;
        }
    }

    return nullptr;
}

void PreprocessorManager::registerInstance(const QString &id)
{
    QMutexLocker locker(mMutex);

    qCDebug(AKONADISERVER_LOG) << "PreprocessorManager::registerInstance(" << id << ")";

    PreprocessorInstance *instance = lockedFindInstance(id);
    if (instance) {
        return; // already registered
    }

    // The PreprocessorInstance objects are actually always added at the end of the queue
    // TODO: Maybe we need some kind of ordering here ?
    //       In that case we'll need to fiddle with the items that are currently enqueued for processing...

    instance = new PreprocessorInstance(id);
    if (!instance->init()) {
        Tracer::self()->warning(
            QStringLiteral("PreprocessorManager"),
            QStringLiteral("Could not initialize preprocessor instance '%1'")
            .arg(id));
        delete instance;
        return;
    }

    qCDebug(AKONADISERVER_LOG) << "Registering preprocessor instance " << id;

    mPreprocessorChain.append(instance);
}

void PreprocessorManager::unregisterInstance(const QString &id)
{
    QMutexLocker locker(mMutex);

    qCDebug(AKONADISERVER_LOG) << "PreprocessorManager::unregisterInstance(" << id << ")";

    lockedUnregisterInstance(id);
}

void PreprocessorManager::lockedUnregisterInstance(const QString &id)
{
    PreprocessorInstance *instance = lockedFindInstance(id);
    if (!instance) {
        return; // not our instance: don't complain (as we might be called for non-preprocessor agents too)
    }

    // All of the preprocessor's waiting items must be queued to the next preprocessor (if there is one)

    std::deque< qint64 > *itemList = instance->itemQueue();
    Q_ASSERT(itemList);

    int idx = mPreprocessorChain.indexOf(instance);
    Q_ASSERT(idx >= 0);   // must be there!

    if (idx < (mPreprocessorChain.count() - 1)) {
        // This wasn't the last preprocessor: trigger the next one.
        PreprocessorInstance *nextPreprocessor = mPreprocessorChain[idx + 1];
        Q_ASSERT(nextPreprocessor);
        Q_ASSERT(nextPreprocessor != instance);

        for (qint64 itemId : *itemList) {
            nextPreprocessor->enqueueItem(itemId);
        }
    } else {
        // This was the last preprocessor: end handling the items
        for (qint64 itemId : *itemList) {
            lockedEndHandleItem(itemId);
        }
    }

    mPreprocessorChain.removeOne(instance);
    delete instance;
}

void PreprocessorManager::beginHandleItem(const PimItem &item, const DataStore *dataStore)
{
    Q_ASSERT(dataStore);
    Q_ASSERT(item.isValid());

    // This is the entry point of the pre-processing chain.
    QMutexLocker locker(mMutex);

    if (!mEnabled) {
        // Preprocessing is disabled: immediately end handling the item.
        // In fact we shouldn't even be here as the caller should
        // have checked isActive() before calling this function.
        // However, since setEnabled() may be called concurrently
        // then this might not be the caller's fault. Just drop a warning.

        qCWarning(AKONADISERVER_LOG) << "PreprocessorManager::beginHandleItem(" << item.id() << ") called with a disabled preprocessor";

        lockedEndHandleItem(item.id());
        return;
    }

#if 0
    // Now the hidden flag is stored as a part.. too hard to assert its existence :D
    Q_ASSERT_X(item.hidden(), "PreprocessorManager::beginHandleItem()", "The item you pass to this function should be hidden!");
#endif

    if (mPreprocessorChain.isEmpty() || CollectionReferenceManager::instance()->isReferenced(item.collectionId())) {
        // No preprocessors at all or referenced collection: immediately end handling the item.
        lockedEndHandleItem(item.id());
        return;
    }

    if (dataStore->inTransaction()) {
        qCDebug(AKONADISERVER_LOG) << "PreprocessorManager::beginHandleItem(" << item.id() << "): the DataStore is in transaction, pushing item to a wait queue";

        // The calling thread data store is in a transaction: push the item into a wait queue
        std::deque< qint64 > *waitQueue = mTransactionWaitQueueHash.value(dataStore, nullptr);

        if (!waitQueue) {
            // No wait queue for this transaction yet...
            waitQueue = new std::deque< qint64 >();

            mTransactionWaitQueueHash.insert(dataStore, waitQueue);

            // This will usually end up being a queued connection.
            QObject::connect(dataStore, &QObject::destroyed, this, &PreprocessorManager::dataStoreDestroyed);
            QObject::connect(dataStore, &DataStore::transactionCommitted, this, &PreprocessorManager::dataStoreTransactionCommitted);
            QObject::connect(dataStore, &DataStore::transactionRolledBack, this, &PreprocessorManager::dataStoreTransactionRolledBack);
        }

        waitQueue->push_back(item.id());

        // nothing more to do here
        return;
    }

    // The calling thread data store is NOT in a transaction: we can proceed directly.
    lockedActivateFirstPreprocessor(item.id());
}

void PreprocessorManager::lockedActivateFirstPreprocessor(qint64 itemId)
{
    // Activate the first preprocessor.
    PreprocessorInstance *preProcessor = mPreprocessorChain.first();
    Q_ASSERT(preProcessor);

    preProcessor->enqueueItem(itemId);
    // The preprocessor will call our "preProcessorFinishedHandlingItem() method"
    // when done with the item.
    //
    // The call should be asynchronous, that is it should never happen that
    // preProcessorFinishedHandlingItem() is called from "inside" enqueueItem()...
    // FIXME: Am I *really* sure of this ? If I'm wrong for some obscure reason then we have a deadlock.
}

void PreprocessorManager::lockedKillWaitQueue(const DataStore *dataStore, bool disconnectSlots)
{
    std::deque< qint64 > *waitQueue = mTransactionWaitQueueHash.value(dataStore, nullptr);
    if (!waitQueue) {
        qCWarning(AKONADISERVER_LOG) << "PreprocessorManager::lockedKillWaitQueue(): called for dataStore which has no wait queue";
        return;
    }

    mTransactionWaitQueueHash.remove(dataStore);

    delete waitQueue;

    if (!disconnectSlots) {
        return;
    }

    QObject::disconnect(dataStore, &QObject::destroyed, this, &PreprocessorManager::dataStoreDestroyed);
    QObject::disconnect(dataStore, &DataStore::transactionCommitted, this, &PreprocessorManager::dataStoreTransactionCommitted);
    QObject::disconnect(dataStore, &DataStore::transactionRolledBack, this, &PreprocessorManager::dataStoreTransactionRolledBack);

}

void PreprocessorManager::dataStoreDestroyed()
{
    QMutexLocker locker(mMutex);

    qCDebug(AKONADISERVER_LOG) << "PreprocessorManager::dataStoreDestroyed(): killing the wait queue";

    const DataStore *dataStore = dynamic_cast< const DataStore *>(sender());
    if (!dataStore) {
        qCWarning(AKONADISERVER_LOG) << "PreprocessorManager::dataStoreDestroyed(): got the signal from a non DataStore object";
        return;
    }

    lockedKillWaitQueue(dataStore, false);   // no need to disconnect slots, qt will do that
}

void PreprocessorManager::dataStoreTransactionCommitted()
{
    QMutexLocker locker(mMutex);

    qCDebug(AKONADISERVER_LOG) << "PreprocessorManager::dataStoreTransactionCommitted(): pushing items in wait queue to the preprocessing chain";

    const DataStore *dataStore = dynamic_cast< const DataStore *>(sender());
    if (!dataStore) {
        qCWarning(AKONADISERVER_LOG) << "PreprocessorManager::dataStoreTransactionCommitted(): got the signal from a non DataStore object";
        return;
    }

    std::deque< qint64 > *waitQueue = mTransactionWaitQueueHash.value(dataStore, nullptr);
    if (!waitQueue) {
        qCWarning(AKONADISERVER_LOG) << "PreprocessorManager::dataStoreTransactionCommitted(): called for dataStore which has no wait queue";
        return;
    }

    if (!mEnabled || mPreprocessorChain.isEmpty()) {
        // Preprocessing has been disabled in the meantime or all the preprocessors died
        for (qint64 id : *waitQueue) {
            lockedEndHandleItem(id);
        }
    } else {
        for (qint64 id : *waitQueue) {
            lockedActivateFirstPreprocessor(id);
        }
    }

    lockedKillWaitQueue(dataStore, true);   // disconnect slots this time
}

void PreprocessorManager::dataStoreTransactionRolledBack()
{
    QMutexLocker locker(mMutex);

    qCDebug(AKONADISERVER_LOG) << "PreprocessorManager::dataStoreTransactionRolledBack(): killing the wait queue";

    const DataStore *dataStore = dynamic_cast< const DataStore *>(sender());
    if (!dataStore) {
        qCWarning(AKONADISERVER_LOG) << "PreprocessorManager::dataStoreTransactionCommitted(): got the signal from a non DataStore object";
        return;
    }

    lockedKillWaitQueue(dataStore, true);   // disconnect slots this time
}

void PreprocessorManager::preProcessorFinishedHandlingItem(PreprocessorInstance *preProcessor, qint64 itemId)
{
    QMutexLocker locker(mMutex);

    int idx = mPreprocessorChain.indexOf(preProcessor);
    Q_ASSERT(idx >= 0);   // must be there!

    if (idx < (mPreprocessorChain.count() - 1)) {
        // This wasn't the last preprocessor: trigger the next one.
        PreprocessorInstance *nextPreprocessor = mPreprocessorChain[idx + 1];
        Q_ASSERT(nextPreprocessor);
        Q_ASSERT(nextPreprocessor != preProcessor);

        nextPreprocessor->enqueueItem(itemId);
    } else {
        // This was the last preprocessor: end handling the item.
        lockedEndHandleItem(itemId);
    }
}

void PreprocessorManager::lockedEndHandleItem(qint64 itemId)
{
    // The exit point of the pre-processing chain.

    // Refetch the PimItem, the Collection and the MimeType now: preprocessing might have changed them.
    PimItem item = PimItem::retrieveById(itemId);
    if (!item.isValid()) {
        // HUM... the preprocessor killed the item ?
        // ... or retrieveById() failed ?
        // Well.. if the preprocessor killed the item then this might be actually OK (spam?).
        qCDebug(AKONADISERVER_LOG) << "Invalid PIM item id '" << itemId << "' passed to preprocessing chain termination function";
        return;
    }

#if 0
    if (!item.hidden()) {
        // HUM... the item was already unhidden for some reason: we have nothing more to do here.
        qCDebug(AKONADISERVER_LOG) << "The PIM item with id '" << itemId << "' reached the preprocessing chain termination function in unhidden state";
        return;
    }
#endif

    if (!DataStore::self()->unhidePimItem(item)) {
        Tracer::self()->warning(
            QStringLiteral("PreprocessorManager"),
            QStringLiteral("Failed to unhide the PIM item '%1': data is not lost but a server restart is required in order to unhide it")
            .arg(itemId));
    }
}

void PreprocessorManager::heartbeat()
{
    QMutexLocker locker(mMutex);

    // Loop through the processor instances and check their current processing time.

    QList< PreprocessorInstance *> firedPreprocessors;

    for (PreprocessorInstance *instance : qAsConst(mPreprocessorChain)) {
        // In this loop we check for "stuck" preprocessors.

        int elapsedTime = instance->currentProcessingTime();

        if (elapsedTime < gWarningItemProcessingTimeInSecs) {
            continue; // ok, still in time.
        }

        // Ooops... the preprocessor looks to be "stuck".
        // This is a rather critical condition and the question is "what we can do about it ?".
        // The fact is that it doesn't really make sense to push another item for
        // processing as the slave process is either dead (silently ?) or stuck anyway.

        // We then proceed as following:
        // - we first kindly ask the preprocessor to abort the job (via Agent.Control interface)
        // - if it doesn't obey after some time we attempt to restart it (via AgentManager)
        // - if it doesn't obey, we drop the interface and assume it's dead until
        //   it's effectively restarted.

        if (elapsedTime < gMaximumItemProcessingTimeInSecs) {
            // Kindly ask the preprocessor to abort the job.

            Tracer::self()->warning(
                QStringLiteral("PreprocessorManager"),
                QStringLiteral("Preprocessor '%1' seems to be stuck... trying to abort its job.")
                .arg(instance->id()));

            if (instance->abortProcessing()) {
                continue;
            }
            // If we're here then abortProcessing() failed.
        }

        if (elapsedTime < gDeadlineItemProcessingTimeInSecs) {
            // Attempt to restart the preprocessor via AgentManager interface

            Tracer::self()->warning(
                QStringLiteral("PreprocessorManager"),
                QStringLiteral("Preprocessor '%1' is stuck... trying to restart it")
                .arg(instance->id()));

            if (instance->invokeRestart()) {
                continue;
            }
            // If we're here then invokeRestart() failed.
        }

        Tracer::self()->warning(
            QStringLiteral("PreprocessorManager"),
            QStringLiteral("Preprocessor '%1' is broken... ignoring it from now on")
            .arg(instance->id()));

        // You're fired! Go Away!
        firedPreprocessors.append(instance);
    }

    // Kill the fired preprocessors, if any.
    for (PreprocessorInstance *instance : qAsConst(firedPreprocessors)) {
        lockedUnregisterInstance(instance->id());
    }
}
