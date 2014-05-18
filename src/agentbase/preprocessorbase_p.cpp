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

#include "dbusconnectionpool.h"
#include "preprocessoradaptor.h"
#include "servermanager.h"

#include "itemfetchjob.h"
#include <KDebug>

using namespace Akonadi;

PreprocessorBasePrivate::PreprocessorBasePrivate(PreprocessorBase *parent)
    : AgentBasePrivate(parent)
    , mInDelayedProcessing(false)
    , mDelayedProcessingItemId(0)
{
    Q_Q(PreprocessorBase);

    new Akonadi__PreprocessorAdaptor(this);

    if (!DBusConnectionPool::threadConnection().registerObject(QStringLiteral("/Preprocessor"), this, QDBusConnection::ExportAdaptors)) {
        q->error(i18n("Unable to register object at dbus: %1", DBusConnectionPool::threadConnection().lastError().message()));
    }

}

void PreprocessorBasePrivate::delayedInit()
{
    if (!DBusConnectionPool::threadConnection().registerService(ServerManager::agentServiceName(ServerManager::Preprocessor, mId))) {
        kFatal() << "Unable to register service at D-Bus: " << DBusConnectionPool::threadConnection().lastError().message();
    }
    AgentBasePrivate::delayedInit();
}

void PreprocessorBasePrivate::beginProcessItem(qlonglong itemId, qlonglong collectionId, const QString &mimeType)
{
    qDebug() << "PreprocessorBase: about to process item " << itemId << " in collection " << collectionId << " with mimeType " << mimeType;

    ItemFetchJob *fetchJob = new ItemFetchJob(Item(itemId), this);
    fetchJob->setFetchScope(mFetchScope);
    connect(fetchJob, SIGNAL(result(KJob*)), SLOT(itemFetched(KJob*)));
}

void PreprocessorBasePrivate::itemFetched(KJob *job)
{
    Q_Q(PreprocessorBase);

    if (job->error()) {
        emit itemProcessed(PreprocessorBase::ProcessingFailed);
        return;
    }

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);

    if (fetchJob->items().isEmpty()) {
        emit itemProcessed(PreprocessorBase::ProcessingFailed);
        return;
    }

    const Item item = fetchJob->items().first();

    switch (q->processItem(item)) {
    case PreprocessorBase::ProcessingFailed:
    case PreprocessorBase::ProcessingRefused:
    case PreprocessorBase::ProcessingCompleted:
        qDebug() << "PreprocessorBase: item processed, emitting signal (" << item.id() << ")";

        // TODO: Handle the different status codes appropriately

        emit itemProcessed(item.id());

        qDebug() << "PreprocessorBase: item processed, signal emitted (" << item.id() << ")";
        break;
    case PreprocessorBase::ProcessingDelayed:
        qDebug() << "PreprocessorBase: item processing delayed (" << item.id() << ")";

        mInDelayedProcessing = true;
        mDelayedProcessingItemId = item.id();
        break;
    }
}
