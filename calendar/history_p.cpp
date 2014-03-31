/*
  Copyright (C) 2010-2012 Sérgio Martins <iamsergio@gmail.com>

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

#include "history_p.h"

#include <kcalutils/stringify.h>
#include <KLocalizedString>

Entry::Entry(const Akonadi::Item &item,
             const QString &description,
             History *qq) : QObject()
{
    mItems << item;
    init(description, qq);
}

Entry::Entry(const Akonadi::Item::List &items,
             const QString &description,
             History *qq) : QObject(), mItems(items)
{
    init(description, qq);
}

void Entry::init(const QString &description, History *qq)
{
    mDescription = description;
    q = qq;
    mChanger = qq->d->mChanger;
}

QWidget* Entry::currentParent() const
{
    return q->d->mCurrentParent;
}

void Entry::updateIds(Item::Id oldId, Item::Id newId)
{
    Q_ASSERT(newId != -1);
    Q_ASSERT(oldId != newId);

    Akonadi::Item::List::iterator it = mItems.begin();
    while (it != mItems.end()) {
        if ((*it).id() == oldId) {
            (*it).setId(newId);
            (*it).setRevision(0);
        }
        ++it;
    }
}

void Entry::updateIdsGlobaly(Item::Id oldId, Item::Id newId)
{
    q->d->updateIds(oldId, newId);
}

void Entry::doIt(OperationType type)
{
    bool result = false;
    mChangeIds.clear();
    if (type == TypeRedo)
        result = redo();
    else if (type == TypeUndo)
        result = undo();
    else
        Q_ASSERT(false);

    if (!result)
        emit finished(IncidenceChanger::ResultCodeJobError, i18n("General error"));
}

CreationEntry::CreationEntry(const Akonadi::Item &item, const QString &description,
                             History *q) : Entry(item, description, q)
{
    mLatestRevisionByItemId.insert(item.id(), item.revision());
    Q_ASSERT(mItems.count() == 1);
    const Incidence::Ptr incidence = mItems.first().payload<KCalCore::Incidence::Ptr>();
    if (mDescription.isEmpty()) {
        mDescription = i18nc("%1 is event, todo or journal", "%1 creation",
                             KCalUtils::Stringify::incidenceType(incidence->type()));
    }
    connect(mChanger, SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onCreateFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));
    connect(mChanger, SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)));
}

bool CreationEntry::undo()
{
    const int changeId = mChanger->deleteIncidence(mItems.first(), currentParent());
    mChangeIds << changeId;

    if (changeId == -1)
        qCritical() << "Undo failed";

    return changeId != -1;
}

bool CreationEntry::redo()
{
    Akonadi::Item item = mItems.first();
    Q_ASSERT(item.hasPayload<KCalCore::Incidence::Ptr>());
    const int changeId = mChanger->createIncidence(item.payload<KCalCore::Incidence::Ptr>(),
                         Collection(item.storageCollectionId()),
                         currentParent());
    mChangeIds << changeId;

    if (changeId == -1)
        qCritical() << "Redo failed";

    return changeId != -1;
}

void CreationEntry::onDeleteFinished(int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                                     Akonadi::IncidenceChanger::ResultCode resultCode,
                                     const QString &errorString)
{
    if (mChangeIds.contains(changeId)) {
        if (resultCode == IncidenceChanger::ResultCodeSuccess) {
            Q_ASSERT(deletedIds.count() == 1);
            mLatestRevisionByItemId.remove(deletedIds.first());   // TODO
        }
        emit finished(resultCode, errorString);
    }
}

void CreationEntry::onCreateFinished(int changeId, const Akonadi::Item &item,
                                     Akonadi::IncidenceChanger::ResultCode resultCode,
                                     const QString &errorString)
{
    if (mChangeIds.contains(changeId)) {
        if (resultCode == IncidenceChanger::ResultCodeSuccess) {
            mLatestRevisionByItemId.insert(item.id(), item.revision());
            Q_ASSERT(mItems.count() == 1);

            if (mItems.first().id() == item.id()) {
                qWarning() << "Duplicate id. Old= " << mItems.first().id() << item.id();
                Q_ASSERT(false);
            }
            updateIdsGlobaly(mItems.first().id(), item.id());
        }
        emit finished(resultCode, errorString);
    }
}

DeletionEntry::DeletionEntry(const Akonadi::Item::List &items, const QString &description,
                             History *q) : Entry(items, description, q)
{
    const Incidence::Ptr incidence = items.first().payload<KCalCore::Incidence::Ptr>();
    if (mDescription.isEmpty()) {
        mDescription = i18nc("%1 is event, todo or journal", "%1 deletion",
                             KCalUtils::Stringify::incidenceType(incidence->type()));
    }
    connect(mChanger, SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onCreateFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));
    connect(mChanger, SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)));
}

bool DeletionEntry::undo()
{
    mResultCode = IncidenceChanger::ResultCodeSuccess;
    mErrorString.clear();
    const bool useAtomicOperation = mItems.count() > 1 ;
    bool success = true;
    foreach(const Akonadi::Item &item, mItems) {
        if (useAtomicOperation)
            mChanger->startAtomicOperation();

        Q_ASSERT(item.hasPayload<KCalCore::Incidence::Ptr>());
        const int changeId = mChanger->createIncidence(item.payload<KCalCore::Incidence::Ptr>(),
                             Collection(item.storageCollectionId()),
                             currentParent());
        success = (changeId != -1) && success;
        mChangeIds << changeId;
        if (useAtomicOperation)
            mChanger->endAtomicOperation();

        mOldIdByChangeId.insert(changeId, item.id());
    }
    mNumPendingCreations = mItems.count();
    return success;
}

bool DeletionEntry::redo()
{
    const int changeId = mChanger->deleteIncidences(mItems, currentParent());
    mChangeIds << changeId;

    if (changeId == -1)
        qCritical() << "Redo failed";

    return changeId != -1;
}

void DeletionEntry::onDeleteFinished(int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                                     Akonadi::IncidenceChanger::ResultCode resultCode,
                                     const QString &errorString)
{
    if (mChangeIds.contains(changeId)) {
        if (resultCode == IncidenceChanger::ResultCodeSuccess) {
            foreach(Akonadi::Item::Id id, deletedIds)
            mLatestRevisionByItemId.remove(id);   // TODO
        }
        emit finished(resultCode, errorString);
    }
}

void DeletionEntry::onCreateFinished(int changeId, const Akonadi::Item &item,
                                     Akonadi::IncidenceChanger::ResultCode resultCode,
                                     const QString &errorString)
{
    if (mChangeIds.contains(changeId)) {
        if (resultCode == IncidenceChanger::ResultCodeSuccess) {
            updateIdsGlobaly(mOldIdByChangeId.value(changeId), item.id());
            mLatestRevisionByItemId.insert(item.id(), item.revision());
        } else {
            mResultCode = resultCode;
            mErrorString = errorString;
        }
        --mNumPendingCreations;
        mOldIdByChangeId.remove(changeId);
        if (mNumPendingCreations == 0)
            emit finished(mResultCode, mErrorString);
    }
}

ModificationEntry::ModificationEntry(const Akonadi::Item &item,
                                     const Incidence::Ptr &originalPayload,
                                     const QString &description,
                                     History *q) : Entry(item, description, q)
    , mOriginalPayload(originalPayload->clone())
{
    const Incidence::Ptr incidence = mItems.first().payload<KCalCore::Incidence::Ptr>();
    if (mDescription.isEmpty()) {
        mDescription =  i18nc("%1 is event, todo or journal", "%1 modification",
                              KCalUtils::Stringify::incidenceType(incidence->type()));
    }

    connect(mChanger, SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onModifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));
}

bool ModificationEntry::undo()
{
    Item oldItem = mItems.first();
    oldItem.setPayload<KCalCore::Incidence::Ptr>(mOriginalPayload);
    const int changeId = mChanger->modifyIncidence(oldItem, Incidence::Ptr(), currentParent());
    mChangeIds << changeId;

    if (changeId == -1)
        qCritical() << "Undo failed";

    return changeId != -1;
}

bool ModificationEntry::redo()
{
    const int changeId = mChanger->modifyIncidence(mItems.first(), mOriginalPayload,
                         currentParent());
    mChangeIds << changeId;

    if (changeId == -1)
        qCritical() << "Redo failed";

    return changeId != -1;
}

void ModificationEntry::onModifyFinished(int changeId, const Akonadi::Item &item,
        Akonadi::IncidenceChanger::ResultCode resultCode,
        const QString &errorString)
{
    if (mChangeIds.contains(changeId)) {
        if (resultCode == IncidenceChanger::ResultCodeSuccess) {
            mLatestRevisionByItemId.insert(item.id(), item.revision());
        }
        emit finished(resultCode, errorString);
    }
}

MultiEntry::MultiEntry(int id, const QString &description,
                       History *q) : Entry(Item(), description, q)
    , mAtomicOperationId(id)
    , mFinishedEntries(0)
    , mOperationInProgress(TypeNone)

{
}

void MultiEntry::addEntry(const Entry::Ptr &entry)
{
    Q_ASSERT(mOperationInProgress == TypeNone);
    mEntries.append(entry);
    connect(entry.data(), SIGNAL(finished(Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(onEntryFinished(Akonadi::IncidenceChanger::ResultCode,QString)),
            Qt::UniqueConnection);
}

void MultiEntry::updateIds(Item::Id oldId, Item::Id newId)
{
    for (int i=0; i<mEntries.count(); ++i) {
        mEntries.at(i)->updateIds(oldId, newId);
    }
}

bool MultiEntry::undo()
{
    mChanger->startAtomicOperation();
    mOperationInProgress = TypeUndo;
    Q_ASSERT(!mEntries.isEmpty());
    mFinishedEntries = 0;

    const int count = mEntries.count();
    // To undo a batch of changes we iterate in reverse order so we don't violate
    // causality.
    for (int i=count-1; i>=0; --i) {
        mEntries[i]->doIt(TypeUndo);
    }

    mChanger->endAtomicOperation();
    return true;
}

bool MultiEntry::redo()
{
    mChanger->startAtomicOperation();
    mOperationInProgress = TypeRedo;
    Q_ASSERT(!mEntries.isEmpty());
    mFinishedEntries = 0;
    foreach(const Entry::Ptr &entry, mEntries)
    entry->doIt(TypeRedo);
    mChanger->endAtomicOperation();
    return true;
}

void MultiEntry::onEntryFinished(Akonadi::IncidenceChanger::ResultCode resultCode,
                                 const QString &errorString)
{
    ++mFinishedEntries;
    if (mFinishedEntries == mEntries.count() ||
            (mFinishedEntries < mEntries.count() &&
             resultCode != IncidenceChanger::ResultCodeSuccess)) {
        mFinishedEntries = mEntries.count(); // we're done
        mOperationInProgress = TypeNone;
        emit finished(resultCode, errorString);
    }
}
