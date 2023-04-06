/*
    SPDX-FileCopyrightText: 2010 KDAB
    SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "conflicthandler_p.h"

#include "itemcreatejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "session.h"
#include <KLocalizedString>

using namespace Akonadi;

ConflictHandler::ConflictHandler(ConflictType type, QObject *parent)
    : QObject(parent)
    , mConflictType(type)
    , mSession(new Session("conflict handling session", this))
{
}

void ConflictHandler::setConflictingItems(const Akonadi::Item &changedItem, const Akonadi::Item &conflictingItem)
{
    mChangedItem = changedItem;
    mConflictingItem = conflictingItem;
}

void ConflictHandler::start()
{
    if (mConflictType == LocalLocalConflict || mConflictType == LocalRemoteConflict) {
        auto job = new ItemFetchJob(mConflictingItem, mSession);
        job->fetchScope().fetchFullPayload();
        job->fetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
        connect(job, &ItemFetchJob::result, this, &ConflictHandler::slotOtherItemFetched);
    } else {
        resolve();
    }
}

void ConflictHandler::slotOtherItemFetched(KJob *job)
{
    if (job->error()) {
        Q_EMIT error(job->errorText()); // TODO: extend error message
        return;
    }

    auto fetchJob = qobject_cast<ItemFetchJob *>(job);
    if (fetchJob->items().isEmpty()) {
        Q_EMIT error(i18n("Did not find other item for conflict handling"));
        return;
    }

    mConflictingItem = fetchJob->items().at(0);
    QMetaObject::invokeMethod(this, &ConflictHandler::resolve, Qt::QueuedConnection);
}

void ConflictHandler::resolve()
{
#pragma message("warning KF5 and KF6 Port me!")
#if 0
    ConflictResolveDialog dlg;
    dlg.setConflictingItems(mChangedItem, mConflictingItem);
    dlg.exec();

    const ResolveStrategy strategy = dlg.resolveStrategy();
    switch (strategy) {
    case UseLocalItem:
        useLocalItem();
        break;
    case UseOtherItem:
        useOtherItem();
        break;
    case UseBothItems:
        useBothItems();
        break;
    }
#endif
}

void ConflictHandler::useLocalItem()
{
    // We have to overwrite the other item inside the Akonadi storage with the local
    // item. To make this happen, we have to set the revision of the local item to
    // the one of the other item to let the Akonadi server accept it.

    Item newItem(mChangedItem);
    newItem.setRevision(mConflictingItem.revision());

    auto job = new ItemModifyJob(newItem, mSession);
    connect(job, &ItemModifyJob::result, this, &ConflictHandler::slotUseLocalItemFinished);
}

void ConflictHandler::slotUseLocalItemFinished(KJob *job)
{
    if (job->error()) {
        Q_EMIT error(job->errorText()); // TODO: extend error message
    } else {
        Q_EMIT conflictResolved();
    }
}

void ConflictHandler::useOtherItem()
{
    // We can just ignore the local item here and leave everything as it is.
    Q_EMIT conflictResolved();
}

void ConflictHandler::useBothItems()
{
    // We have to create a new item for the local item under the collection that has
    // been retrieved when we fetched the other item.
    auto job = new ItemCreateJob(mChangedItem, mConflictingItem.parentCollection(), mSession);
    connect(job, &ItemCreateJob::result, this, &ConflictHandler::slotUseBothItemsFinished);
}

void ConflictHandler::slotUseBothItemsFinished(KJob *job)
{
    if (job->error()) {
        Q_EMIT error(job->errorText()); // TODO: extend error message
    } else {
        Q_EMIT conflictResolved();
    }
}

#include "moc_conflicthandler_p.cpp"
