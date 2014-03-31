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

#include "history.h"
#include "history_p.h"

#include <kcalutils/stringify.h>

using namespace KCalCore;
using namespace Akonadi;

History::History(QObject *parent) : QObject(parent), d(new Private(this))
{
    d->mChanger = new IncidenceChanger(/*history=*/false, this);
    d->mChanger->setObjectName("changer");   // for auto-connects
    d->mOperationTypeInProgress = TypeNone;
    d->mEnabled = true;
    d->mUndoAllInProgress = false;
}

History::~History()
{
    delete d;
}

History::Private::Private(History *qq) : q(qq)
{

}

void History::recordCreation(const Akonadi::Item &item,
                             const QString &description,
                             const uint atomicOperationId)
{
    Q_ASSERT_X(item.isValid(), "History::recordCreation()",
               "Item must be valid.");

    Q_ASSERT_X(item.hasPayload<KCalCore::Incidence::Ptr>(), "History::recordCreation()",
               "Item must have Incidence::Ptr payload.");

    Entry::Ptr entry(new CreationEntry(item, description, this));

    d->stackEntry(entry, atomicOperationId);
}

void History::recordModification(const Akonadi::Item &oldItem,
                                 const Akonadi::Item &newItem,
                                 const QString &description,
                                 const uint atomicOperationId)
{
    Q_ASSERT_X(oldItem.isValid(), "History::recordModification", "old item must be valid");
    Q_ASSERT_X(newItem.isValid(), "History::recordModification", "newItem item must be valid");
    Q_ASSERT_X(oldItem.hasPayload<KCalCore::Incidence::Ptr>(), "History::recordModification",
               "old item must have Incidence::Ptr payload");
    Q_ASSERT_X(newItem.hasPayload<KCalCore::Incidence::Ptr>(), "History::recordModification",
               "newItem item must have Incidence::Ptr payload");

    Entry::Ptr entry(new ModificationEntry(newItem, oldItem.payload<KCalCore::Incidence::Ptr>(),
                                           description, this));

    Q_ASSERT(newItem.revision() >= oldItem.revision());

    d->stackEntry(entry, atomicOperationId);
}

void History::recordDeletion(const Akonadi::Item &item,
                             const QString &description,
                             const uint atomicOperationId)
{
    Q_ASSERT_X(item.isValid(), "History::recordDeletion", "Item must be valid");
    Item::List list;
    list.append(item);
    recordDeletions(list, description, atomicOperationId);
}

void History::recordDeletions(const Akonadi::Item::List &items,
                              const QString &description,
                              const uint atomicOperationId)
{
    Entry::Ptr entry(new DeletionEntry(items, description, this));

    foreach(const Akonadi::Item &item, items) {
        Q_UNUSED(item);
        Q_ASSERT_X(item.isValid(),
                   "History::recordDeletion()", "Item must be valid.");
        Q_ASSERT_X(item.hasPayload<Incidence::Ptr>(),
                   "History::recordDeletion()", "Item must have an Incidence::Ptr payload.");
    }

    d->stackEntry(entry, atomicOperationId);
}

QString History::nextUndoDescription() const
{
    if (!d->mUndoStack.isEmpty())
        return d->mUndoStack.top()->mDescription;
    else
        return QString();
}

QString History::nextRedoDescription() const
{
    if (!d->mRedoStack.isEmpty())
        return d->mRedoStack.top()->mDescription;
    else
        return QString();
}

void History::undo(QWidget *parent)
{
    d->undoOrRedo(TypeUndo, parent);
}

void History::redo(QWidget *parent)
{
    d->undoOrRedo(TypeRedo, parent);
}

void History::undoAll(QWidget *parent)
{
    if (d->mOperationTypeInProgress != TypeNone) {
        qWarning() << "Don't call History::undoAll() while an undo/redo/undoAll is in progress";
    } else if (d->mEnabled) {
        d->mUndoAllInProgress = true;
        d->mCurrentParent = parent;
        d->doIt(TypeUndo);
    } else {
        qWarning() << "Don't call undo/redo when History is disabled";
    }
}

bool History::clear()
{
    bool result = true;
    if (d->mOperationTypeInProgress == TypeNone) {
        d->mRedoStack.clear();
        d->mUndoStack.clear();
        d->mLastErrorString.clear();
        d->mQueuedEntries.clear();
    } else {
        result = false;
    }
    emit changed();
    return result;
}

QString History::lastErrorString() const
{
    return d->mLastErrorString;
}

bool History::undoAvailable() const
{
    return !d->mUndoStack.isEmpty() && d->mOperationTypeInProgress == TypeNone;
}

bool History::redoAvailable() const
{
    return !d->mRedoStack.isEmpty() && d->mOperationTypeInProgress == TypeNone;
}

void History::Private::updateIds(Item::Id oldId, Item::Id newId)
{
    mEntryInProgress->updateIds(oldId, newId);

    foreach(const Entry::Ptr &entry, mUndoStack)
    entry->updateIds(oldId, newId);

    foreach(const Entry::Ptr &entry, mRedoStack)
    entry->updateIds(oldId, newId);
}

void History::Private::doIt(OperationType type)
{
    mOperationTypeInProgress = type;
    emit q->changed(); // Application will disable undo/redo buttons because operation is in progress
    Q_ASSERT(!stack().isEmpty());
    mEntryInProgress = stack().pop();

    connect(mEntryInProgress.data(), SIGNAL(finished(Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(handleFinished(Akonadi::IncidenceChanger::ResultCode,QString)),
            Qt::UniqueConnection);
    mEntryInProgress->doIt(type);
}

void History::Private::handleFinished(IncidenceChanger::ResultCode changerResult,
                                      const QString &errorString)
{
    Q_ASSERT(mOperationTypeInProgress != TypeNone);
    Q_ASSERT(!(mUndoAllInProgress && mOperationTypeInProgress == TypeRedo));

    const bool success = (changerResult == IncidenceChanger::ResultCodeSuccess);
    const History::ResultCode resultCode = success ? History::ResultCodeSuccess :
                                           History::ResultCodeError;

    if (success) {
        mLastErrorString.clear();
        destinationStack().push(mEntryInProgress);
    } else {
        mLastErrorString = errorString;
        stack().push(mEntryInProgress);
    }

    mCurrentParent = 0;

    // Process recordCreation/Modification/Deletions that came in while an operation
    // was in progress
    if (!mQueuedEntries.isEmpty()) {
        mRedoStack.clear();
        foreach(const Entry::Ptr &entry, mQueuedEntries) {
            mUndoStack.push(entry);
        }
        mQueuedEntries.clear();
    }

    emitDone(mOperationTypeInProgress, resultCode);
    mOperationTypeInProgress = TypeNone;
    emit q->changed();
}

void History::Private::stackEntry(const Entry::Ptr &entry, uint atomicOperationId)
{
    const bool useMultiEntry = (atomicOperationId > 0);

    Entry::Ptr entryToPush;

    if (useMultiEntry) {
        Entry::Ptr topEntry = (mOperationTypeInProgress == TypeNone)                           ?
                              (mUndoStack.isEmpty() ? Entry::Ptr() : mUndoStack.top())   :
                                  (mQueuedEntries.isEmpty() ? Entry::Ptr() : mQueuedEntries.last());

        const bool topIsMultiEntry = qobject_cast<MultiEntry*>(topEntry.data());

        if (topIsMultiEntry) {
            MultiEntry::Ptr multiEntry = topEntry.staticCast<MultiEntry>();
            if (multiEntry->mAtomicOperationId != atomicOperationId) {
                multiEntry = MultiEntry::Ptr(new MultiEntry(atomicOperationId, entry->mDescription, q));
                entryToPush = multiEntry;
            }
            multiEntry->addEntry(entry);
        } else {
            MultiEntry::Ptr multiEntry = MultiEntry::Ptr(new MultiEntry(atomicOperationId,
                                         entry->mDescription, q));
            multiEntry->addEntry(entry);
            entryToPush = multiEntry;
        }
    } else {
        entryToPush = entry;
    }

    if (mOperationTypeInProgress == TypeNone) {
        if (entryToPush) {
            mUndoStack.push(entryToPush);
        }
        mRedoStack.clear();
        emit q->changed();
    } else {
        if (entryToPush) {
            mQueuedEntries.append(entryToPush);
        }
    }
}

void History::Private::undoOrRedo(OperationType type, QWidget *parent)
{
    // Don't call undo() without the previous one finishing
    Q_ASSERT(mOperationTypeInProgress == TypeNone);

    if (!stack(type).isEmpty()) {
        if (mEnabled) {
            mCurrentParent = parent;
            doIt(type);
        } else {
            qWarning() << "Don't call undo/redo when History is disabled";
        }
    } else {
        qWarning() << "Don't call undo/redo when the stack is empty.";
    }
}

QStack<Entry::Ptr>& History::Private::stack(OperationType type)
{
    // Entries from the undo stack go to the redo stack, and vice-versa
    return type == TypeUndo ? mUndoStack : mRedoStack;
}

void History::Private::setEnabled(bool enabled)
{
    if (enabled != mEnabled) {
        mEnabled = enabled;
    }
}

int History::Private::redoCount() const
{
    return mRedoStack.count();
}

int History::Private::undoCount() const
{
    return mUndoStack.count();
}

QStack<Entry::Ptr>& History::Private::stack()
{
    return stack(mOperationTypeInProgress);
}

QStack<Entry::Ptr>& History::Private::destinationStack()
{
    // Entries from the undo stack go to the redo stack, and vice-versa
    return mOperationTypeInProgress == TypeRedo ? mUndoStack : mRedoStack;
}

void History::Private::emitDone(OperationType type, History::ResultCode resultCode)
{
    if (type == TypeUndo) {
        emit q->undone(resultCode);
    } else if (type == TypeRedo) {
        emit q->redone(resultCode);
    } else {
        Q_ASSERT(false);
    }
}

#include "moc_history.cpp"
#include "moc_history_p.cpp"
