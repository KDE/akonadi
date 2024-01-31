/******************************************************************************
 *
 *  File : preprocessorinstance.cpp
 *  Creation date : Sat 18 Jul 2009 02:50:39
 *
 *  SPDX-FileCopyrightText: 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 *
 *****************************************************************************/

#include "preprocessorinstance.h"
#include "akonadiserver_debug.h"
#include "preprocessorinterface.h"
#include "preprocessormanager.h"

#include "entities.h"

#include "agentcontrolinterface.h"
#include "agentmanagerinterface.h"

#include "tracer.h"

#include "private/dbus_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

PreprocessorInstance::PreprocessorInstance(const QString &id, PreprocessorManager &manager, Tracer &tracer)
    : mManager(manager)
    , mTracer(tracer)
    , mId(id)
{
    Q_ASSERT(!id.isEmpty());
}

PreprocessorInstance::~PreprocessorInstance() = default;

bool PreprocessorInstance::init()
{
    Q_ASSERT(!mBusy); // must be called very early
    Q_ASSERT(!mInterface);

    mInterface = new OrgFreedesktopAkonadiPreprocessorInterface(DBus::agentServiceName(mId, DBus::Preprocessor),
                                                                QStringLiteral("/Preprocessor"),
                                                                QDBusConnection::sessionBus(),
                                                                this);

    if (!mInterface || !mInterface->isValid()) {
        mTracer.warning(
            QStringLiteral("PreprocessorInstance"),
            QStringLiteral("Could not connect to pre-processor instance '%1': %2").arg(mId, mInterface ? mInterface->lastError().message() : QString()));
        delete mInterface;
        mInterface = nullptr;
        return false;
    }

    QObject::connect(mInterface, &OrgFreedesktopAkonadiPreprocessorInterface::itemProcessed, this, &PreprocessorInstance::itemProcessed);

    return true;
}

void PreprocessorInstance::enqueueItem(qint64 itemId)
{
    qCDebug(AKONADISERVER_LOG) << "PreprocessorInstance::enqueueItem(" << itemId << ")";

    mItemQueue.push_back(itemId);

    // If the preprocessor is already busy processing another item then do nothing.
    if (mBusy) {
        // The "head" item is the one being processed and we have just added another one.
        Q_ASSERT(mItemQueue.size() > 1);
        return;
    }

    // Not busy: handle the item.
    processHeadItem();
}

void PreprocessorInstance::processHeadItem()
{
    // We shouldn't be called if there are no items in the queue
    Q_ASSERT(!mItemQueue.empty());
    // We shouldn't be here with no interface
    Q_ASSERT(mInterface);

    qint64 itemId = mItemQueue.front();

    // Fetch the actual item data (as it may have changed since it was enqueued)
    // The fetch will hit the cache if the item wasn't changed.

    PimItem actualItem = PimItem::retrieveById(itemId);

    while (!actualItem.isValid()) {
        // hum... item is gone ?
        mManager.preProcessorFinishedHandlingItem(this, itemId);

        mItemQueue.pop_front();
        if (mItemQueue.empty()) {
            // nothing more to process for this instance: jump out
            mBusy = false;
            return;
        }

        // try the next one in the queue
        itemId = mItemQueue.front();
        actualItem = PimItem::retrieveById(itemId);
    }

    // Ok.. got a valid item to process: collection and mimetype is known.

    qCDebug(AKONADISERVER_LOG) << "PreprocessorInstance::processHeadItem(): about to begin processing item " << itemId;

    mBusy = true;

    mItemProcessingStartDateTime = QDateTime::currentDateTime();

    // The beginProcessItem() D-Bus call is asynchronous (marked with NoReply attribute)
    mInterface->beginProcessItem(itemId, actualItem.collectionId(), actualItem.mimeType().name());

    qCDebug(AKONADISERVER_LOG) << "PreprocessorInstance::processHeadItem(): processing started for item " << itemId;
}

qint64 PreprocessorInstance::currentProcessingTime()
{
    if (!mBusy) {
        return -1; // nothing being processed
    }

    return mItemProcessingStartDateTime.secsTo(QDateTime::currentDateTime());
}

bool PreprocessorInstance::abortProcessing()
{
    Q_ASSERT_X(mBusy, "PreprocessorInstance::abortProcessing()", "You shouldn't call this method when isBusy() returns false");

    OrgFreedesktopAkonadiAgentControlInterface iface(DBus::agentServiceName(mId, DBus::Agent), QStringLiteral("/"), QDBusConnection::sessionBus(), this);

    if (!iface.isValid()) {
        mTracer.warning(QStringLiteral("PreprocessorInstance"),
                        QStringLiteral("Could not connect to pre-processor instance '%1': %2").arg(mId, iface.lastError().message()));
        return false;
    }

    // We don't check the return value.. as this is a "warning"
    // The preprocessor manager will check again in a while and eventually
    // terminate the agent at all...
    iface.abort();

    return true;
}

bool PreprocessorInstance::invokeRestart()
{
    Q_ASSERT_X(mBusy, "PreprocessorInstance::invokeRestart()", "You shouldn't call this method when isBusy() returns false");

    OrgFreedesktopAkonadiAgentManagerInterface iface(DBus::serviceName(DBus::Control), QStringLiteral("/AgentManager"), QDBusConnection::sessionBus(), this);

    if (!iface.isValid()) {
        mTracer.warning(
            QStringLiteral("PreprocessorInstance"),
            QStringLiteral("Could not connect to the AgentManager in order to restart pre-processor instance '%1': %2").arg(mId, iface.lastError().message()));
        return false;
    }

    iface.restartAgentInstance(mId);

    return true;
}

void PreprocessorInstance::itemProcessed(qlonglong id)
{
    qCDebug(AKONADISERVER_LOG) << "PreprocessorInstance::itemProcessed(" << id << ")";

    // We shouldn't be called if there are no items in the queue
    if (mItemQueue.empty()) {
        mTracer.warning(QStringLiteral("PreprocessorInstance"),
                        QStringLiteral("Pre-processor instance '%1' emitted itemProcessed(%2) but we actually have no item in the queue").arg(mId).arg(id));
        mBusy = false;
        return; // preprocessor is buggy (FIXME: What now ?)
    }

    // We should be busy now: this is more likely our fault, not the preprocessor's one.
    Q_ASSERT(mBusy);

    qlonglong itemId = mItemQueue.front();

    if (itemId != id) {
        mTracer.warning(
            QStringLiteral("PreprocessorInstance"),
            QStringLiteral("Pre-processor instance '%1' emitted itemProcessed(%2) but the head item in the queue has id %3").arg(mId).arg(id).arg(itemId));

        // FIXME: And what now ?
    }

    mItemQueue.pop_front();

    mManager.preProcessorFinishedHandlingItem(this, itemId);

    if (mItemQueue.empty()) {
        // Nothing more to do
        mBusy = false;
        return;
    }

    // Stay busy and process next item in the queue
    processHeadItem();
}

#include "moc_preprocessorinstance.cpp"
