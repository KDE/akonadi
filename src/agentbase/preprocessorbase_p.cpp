/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "preprocessorbase_p.h"
#include "preprocessorbase.h"

#include "KDBusConnectionPool"
#include "preprocessoradaptor.h"
#include "servermanager.h"

#include "itemfetchjob.h"
#include "akonadiagentbase_debug.h"

using namespace Akonadi;

PreprocessorBasePrivate::PreprocessorBasePrivate(PreprocessorBase *parent)
    : AgentBasePrivate(parent)
    , mInDelayedProcessing(false)
    , mDelayedProcessingItemId(0)
{
    Q_Q(PreprocessorBase);

    new Akonadi__PreprocessorAdaptor(this);

    if (!KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/Preprocessor"), this, QDBusConnection::ExportAdaptors)) {
        Q_EMIT q->error(i18n("Unable to register object at dbus: %1", KDBusConnectionPool::threadConnection().lastError().message()));
    }

}

void PreprocessorBasePrivate::delayedInit()
{
    if (!KDBusConnectionPool::threadConnection().registerService(ServerManager::agentServiceName(ServerManager::Preprocessor, mId))) {
        qCCritical(AKONADIAGENTBASE_LOG) << "Unable to register service at D-Bus: "
                                         << KDBusConnectionPool::threadConnection().lastError().message();
    }
    AgentBasePrivate::delayedInit();
}

void PreprocessorBasePrivate::beginProcessItem(qlonglong itemId, qlonglong collectionId, const QString &mimeType)
{
    qCDebug(AKONADIAGENTBASE_LOG) << "PreprocessorBase: about to process item " << itemId << " in collection " << collectionId << " with mimeType " << mimeType;

    ItemFetchJob *fetchJob = new ItemFetchJob(Item(itemId), this);
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

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);

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
