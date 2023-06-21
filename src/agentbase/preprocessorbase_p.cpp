/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "preprocessorbase_p.h"

#include "preprocessoradaptor.h"
#include "servermanager.h"
#include <QDBusConnection>

#include "akonadiagentbase_debug.h"
#include "itemfetchjob.h"

using namespace Akonadi;

PreprocessorBasePrivate::PreprocessorBasePrivate(PreprocessorBase *parent)
    : AgentBasePrivate(parent)
{
    Q_Q(PreprocessorBase);

    new Akonadi__PreprocessorAdaptor(this);

    if (!QDBusConnection::sessionBus().registerObject(QStringLiteral("/Preprocessor"), this, QDBusConnection::ExportAdaptors)) {
        Q_EMIT q->error(i18n("Unable to register object at dbus: %1", QDBusConnection::sessionBus().lastError().message()));
    }
}

void PreprocessorBasePrivate::delayedInit()
{
    if (!QDBusConnection::sessionBus().registerService(ServerManager::agentServiceName(ServerManager::Preprocessor, mId))) {
        qCCritical(AKONADIAGENTBASE_LOG) << "Unable to register service at D-Bus: " << QDBusConnection::sessionBus().lastError().message();
    }
    AgentBasePrivate::delayedInit();
}

void PreprocessorBasePrivate::beginProcessItem(qlonglong itemId, qlonglong collectionId, const QString &mimeType)
{
    qCDebug(AKONADIAGENTBASE_LOG) << "PreprocessorBase: about to process item " << itemId << " in collection " << collectionId << " with mimeType " << mimeType;

    auto fetchJob = new ItemFetchJob(Item(itemId), this);
    fetchJob->setFetchScope(mFetchScope);
    connect(fetchJob, &ItemFetchJob::result, this, &PreprocessorBasePrivate::itemFetched);
}

void PreprocessorBasePrivate::itemFetched(KJob *job)
{
    Q_Q(PreprocessorBase);

    if (job->error()) {
        Q_EMIT itemProcessed(PreprocessorBase::ProcessingFailed);
        return;
    }

    auto fetchJob = qobject_cast<ItemFetchJob *>(job);

    if (fetchJob->items().isEmpty()) {
        Q_EMIT itemProcessed(PreprocessorBase::ProcessingFailed);
        return;
    }

    const Item item = fetchJob->items().at(0);

    switch (q->processItem(item)) {
    case PreprocessorBase::ProcessingFailed:
    case PreprocessorBase::ProcessingRefused:
    case PreprocessorBase::ProcessingCompleted:
        qCDebug(AKONADIAGENTBASE_LOG) << "PreprocessorBase: item processed, emitting signal (" << item.id() << ")";

        // TODO: Handle the different status codes appropriately

        Q_EMIT itemProcessed(item.id());

        qCDebug(AKONADIAGENTBASE_LOG) << "PreprocessorBase: item processed, signal emitted (" << item.id() << ")";
        break;
    case PreprocessorBase::ProcessingDelayed:
        qCDebug(AKONADIAGENTBASE_LOG) << "PreprocessorBase: item processing delayed (" << item.id() << ")";

        mInDelayedProcessing = true;
        mDelayedProcessingItemId = item.id();
        break;
    }
}

#include "moc_preprocessorbase_p.cpp"
