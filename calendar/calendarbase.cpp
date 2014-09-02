/*
   Copyright (C) 2011 Sérgio Martins <sergio.martins@kdab.com>
   Copyright (C) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#include "calendarbase.h"
#include "calendarbase_p.h"
#include "incidencechanger.h"
#include "utils_p.h"

#include <Akonadi/CollectionFetchJob>
#include <akonadi/item.h>
#include <akonadi/collection.h>

#include <KSystemTimeZones>
#include <KLocalizedString>

using namespace Akonadi;
using namespace KCalCore;

static QString itemToString(const Akonadi::Item &item)
{
    const KCalCore::Incidence::Ptr &incidence = CalendarUtils::incidence(item);
    QString str;
    QTextStream stream(&str);
    stream << item.id()
           << "; summary=" << incidence->summary() << "; uid=" << incidence->uid() << "; type="
           << incidence->type() << "; recurs=" << incidence->recurs() << "; recurrenceId="
           << incidence->recurrenceId().toString() << "; dtStart=" << incidence->dtStart().toString()
           << "; dtEnd=" << incidence->dateTime(Incidence::RoleEnd).toString()
           << "; parentCollection=" << item.storageCollectionId() << item.parentCollection().displayName();

    return str;
}

CalendarBasePrivate::CalendarBasePrivate(CalendarBase *qq) : QObject()
    , mIncidenceChanger(new IncidenceChanger())
    , mBatchInsertionCancelled(false)
    , mListensForNewItems(false)
    , mLastCreationCancelled(false)
    , q(qq)
{
    connect(mIncidenceChanger,
            SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(slotCreateFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));

    connect(mIncidenceChanger,
            SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(slotDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)));

    connect(mIncidenceChanger,
            SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
            SLOT(slotModifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));

    mIncidenceChanger->setDestinationPolicy(IncidenceChanger::DestinationPolicyAsk);
    mIncidenceChanger->setGroupwareCommunication(false);
    mIncidenceChanger->setHistoryEnabled(false);
}

CalendarBasePrivate::~CalendarBasePrivate()
{
    delete mIncidenceChanger;
    mIncidenceChanger = 0;
}

void CalendarBasePrivate::internalInsert(const Akonadi::Item &item)
{
    Q_ASSERT(item.isValid());
    Q_ASSERT(item.hasPayload<KCalCore::Incidence::Ptr>());
    KCalCore::Incidence::Ptr incidence = CalendarUtils::incidence(item);

    if (!incidence) {
        kError() << "Incidence is null. id=" << item.id()
                 << "; hasPayload()="  << item.hasPayload()
                 << "; has incidence=" << item.hasPayload<KCalCore::Incidence::Ptr>()
                 << "; mime type="     << item.mimeType();
        Q_ASSERT(false);
        return;
    }

    //kDebug() << "Inserting incidence in calendar. id=" << item.id() << "uid=" << incidence->uid();
    const QString uid = incidence->instanceIdentifier();

    if (uid.isEmpty()) {
        // This code path should never happen
        kError() << "Incidence has empty UID. id=" << item.id()
                 << "; summary=" << incidence->summary()
                 << "Please fix it. Ignoring this incidence.";
        return;
    }

    if (mItemIdByUid.contains(uid) && mItemIdByUid[uid] != item.id()) {
        // We only allow duplicate UIDs if they have the same item id, for example
        // when using virtual folders.
        kWarning() << "Discarding duplicate incidence with instanceIdentifier=" << uid
                   << "and summary " << incidence->summary()
                   << "; recurrenceId() =" << incidence->recurrenceId()
                   << "; new id=" << item.id()
                   << "; existing id=" << mItemIdByUid[uid];
        return;
    }

    if (incidence->type() == KCalCore::Incidence::TypeEvent && !incidence->dtStart().isValid()) {
        // TODO: make the parser discard them would also be a good idea
        kWarning() << "Discarding event with invalid DTSTART. identifier="
                   << incidence->instanceIdentifier() << "; summary=" << incidence->summary();
        return;
    }

    Akonadi::Collection collection = item.parentCollection();
    if (collection.isValid()) {
        // Some items don't have collection set
        if (item.storageCollectionId() !=  collection.id() && item.storageCollectionId() > -1) {
            if (mCollections.contains(item.storageCollectionId())) {
                collection = mCollections.value(item.storageCollectionId());
                incidence->setReadOnly(!(collection.rights() & Akonadi::Collection::CanChangeItem));
              } else if (!mCollectionJobs.key(item.storageCollectionId())) {
                collection = Akonadi::Collection(item.storageCollectionId());
                Akonadi::CollectionFetchJob *job = new Akonadi::CollectionFetchJob(collection, Akonadi::CollectionFetchJob::Base, this);
                connect(job, SIGNAL(result(KJob*)), this, SLOT(collectionFetchResult(KJob*)));
                mCollectionJobs.insert(job,  collection.id());
            }
        } else {
            mCollections.insert(collection.id(), collection);
            incidence->setReadOnly(!(collection.rights() & Akonadi::Collection::CanChangeItem));
        }
    }

    mItemById.insert(item.id(), item);
    mItemIdByUid.insert(uid, item.id());
    mItemsByCollection.insert(item.storageCollectionId(), item);

    if (!incidence->hasRecurrenceId()) {
        // Insert parent relationships
        const QString parentUid = incidence->relatedTo();
        if (!parentUid.isEmpty()) {
            mParentUidToChildrenUid[parentUid].append(incidence->uid());
            mUidToParent.insert(uid, parentUid);
        }
    }

    incidence->setCustomProperty("VOLATILE", "AKONADI-ID", QString::number(item.id()));
    // Must be the last one due to re-entrancy
    const bool result = q->MemoryCalendar::addIncidence(incidence);
    if (!result) {
        kError() << "Error adding incidence " << itemToString(item);
        Q_ASSERT(false);
    }
}

void CalendarBasePrivate::collectionFetchResult(KJob* job)
{
    Akonadi::Collection::Id colid = mCollectionJobs.take(job);

    if ( job->error() ) {
        qDebug() << "Error occurred";
        return;
    }

    Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );

    const Akonadi::Collection collection = fetchJob->collections().first();
    if (collection.id() !=  colid) {
        kError() <<  "Fetched the wrong collection,  should fetch: " <<  colid << "fetched: " <<  collection.id();
    }

    bool isReadOnly = !(collection.rights() & Akonadi::Collection::CanChangeItem);
    foreach (const Akonadi::Item &item, mItemsByCollection.values(collection.id())) {
        KCalCore::Incidence::Ptr incidence = CalendarUtils::incidence(item);
        incidence->setReadOnly(isReadOnly);
    }

    mCollections.insert(collection.id(),  collection);

    if (mCollectionJobs.count() == 0) {
        emit fetchFinished();
    }
}

void CalendarBasePrivate::internalRemove(const Akonadi::Item &item)
{
    Q_ASSERT(item.isValid());

    Incidence::Ptr tmp = CalendarUtils::incidence(item);
    if (!tmp) {
        kError() << "CalendarBase::internalRemove1: incidence is null, item.id=" << item.id();
        return;
    }

    // We want the one stored in the calendar
    Incidence::Ptr incidence = q->incidence(tmp->uid(), tmp->recurrenceId());

    // Null incidence means it was deleted via CalendarBase::deleteIncidence(), but then
    // the ETMCalendar received the monitor notification and tried to delete it again.
    if (incidence) {
        mItemById.remove(item.id());
        // kDebug() << "Deleting incidence from calendar .id=" << item.id() << "uid=" << incidence->uid();
        mItemIdByUid.remove(incidence->instanceIdentifier());

        mItemsByCollection.remove(item.storageCollectionId(), item);

        if (!incidence->hasRecurrenceId()) {
            const QString uid = incidence->uid();
            const QString parentUid = incidence->relatedTo();
            mParentUidToChildrenUid.remove(uid);
            if (!parentUid.isEmpty()) {
                mParentUidToChildrenUid[parentUid].removeAll(uid);
                mUidToParent.remove(uid);
            }
        }
        // Must be the last one due to re-entrancy
        const bool result = q->MemoryCalendar::deleteIncidence(incidence);
        if (!result) {
            kError() << "Error removing incidence " << itemToString(item);
            Q_ASSERT(false);
        }
    } else {
        kWarning() << "CalendarBase::internalRemove2: incidence is null, item.id=" << itemToString(item);
    }
}

void CalendarBasePrivate::deleteAllIncidencesOfType(const QString &mimeType)
{
    kWarning() << "Refusing to delete your Incidences.";
    Q_UNUSED(mimeType);
    /*
    QHash<Item::Id, Item>::iterator i;
    Item::List incidences;
    for ( i = mItemById.begin(); i != mItemById.end(); ++i ) {
      if ( i.value().payload<KCalCore::Incidence::Ptr>()->mimeType() == mimeType )
        incidences.append( i.value() );
    }

    mIncidenceChanger->deleteIncidences( incidences );
    */
}

void CalendarBasePrivate::slotDeleteFinished(int changeId,
        const QVector<Akonadi::Item::Id> &itemIds,
        IncidenceChanger::ResultCode resultCode,
        const QString &errorMessage)
{
    Q_UNUSED(changeId);
    if (resultCode == IncidenceChanger::ResultCodeSuccess) {
        foreach(const Akonadi::Item::Id &id, itemIds) {
            if (mItemById.contains(id))
                internalRemove(mItemById.value(id));
        }
    }

    emit q->deleteFinished(resultCode == IncidenceChanger::ResultCodeSuccess, errorMessage);
}

void CalendarBasePrivate::slotCreateFinished(int changeId,
        const Akonadi::Item &item,
        IncidenceChanger::ResultCode resultCode,
        const QString &errorMessage)
{
    Q_UNUSED(changeId);
    Q_UNUSED(item);
    if (resultCode == IncidenceChanger::ResultCodeSuccess && !mListensForNewItems) {
        Q_ASSERT(item.isValid());
        Q_ASSERT(item.hasPayload<KCalCore::Incidence::Ptr>());
        internalInsert(item);
    }

    mLastCreationCancelled = (resultCode == IncidenceChanger::ResultCodeUserCanceled);

    emit q->createFinished(resultCode == IncidenceChanger::ResultCodeSuccess, errorMessage);
}

void CalendarBasePrivate::slotModifyFinished(int changeId,
        const Akonadi::Item &item,
        IncidenceChanger::ResultCode resultCode,
        const QString &errorMessage)
{
    Q_UNUSED(changeId);
    Q_UNUSED(item);
    QString message = errorMessage;
    if (resultCode == IncidenceChanger::ResultCodeSuccess) {
        KCalCore::Incidence::Ptr incidence = CalendarUtils::incidence(item);
        Q_ASSERT(incidence);
        KCalCore::Incidence::Ptr localIncidence = q->incidence(incidence->instanceIdentifier());

        if (localIncidence) {
            //update our local one
            *(static_cast<KCalCore::IncidenceBase*>(localIncidence.data())) = *(incidence.data());
        } else {
            // This shouldn't happen, unless the incidence gets deleted between event loops
            kWarning() << "CalendarBasePrivate::slotModifyFinished() Incidence was deleted already probably? id=" << item.id();
            message = i18n("Could not find incidence to update, it probably was deleted recently.");
            resultCode = IncidenceChanger::ResultCodeAlreadyDeleted;
        }
    }
    emit q->modifyFinished(resultCode == IncidenceChanger::ResultCodeSuccess, message);
}

void CalendarBasePrivate::handleUidChange(const Akonadi::Item &oldItem,
        const Akonadi::Item &newItem, const QString &newIdentifier)
{
    Q_ASSERT(oldItem.isValid());
    Incidence::Ptr newIncidence = CalendarUtils::incidence(newItem);
    Q_ASSERT(newIncidence);
    Incidence::Ptr oldIncidence = CalendarUtils::incidence(oldItem);
    Q_ASSERT(oldIncidence);

    const QString newUid = newIncidence->uid();
    if (mItemIdByUid.contains(newIdentifier)) {
        Incidence::Ptr oldIncidence = CalendarUtils::incidence(oldItem);
        kWarning() << "New uid shouldn't be known: "  << newIdentifier << "; id="
                   << newItem.id() << "; oldItem.id=" << mItemIdByUid[newIdentifier]
                   << "; new summary= " << newIncidence->summary()
                   << "; new recurrenceId=" << newIncidence->recurrenceId()
                   << "; oldIncidence" << oldIncidence;
        if (oldIncidence) {
            kWarning() << "; oldIncidence uid=" << oldIncidence->uid()
                       << "; oldIncidence recurrenceId = " << oldIncidence->recurrenceId()
                       << "; oldIncidence summary = " << oldIncidence->summary();
        }
        Q_ASSERT(false);
        return;
    }

    mItemIdByUid[newIdentifier] = newItem.id();

    // Get the real pointer
    oldIncidence = q->MemoryCalendar::incidence(oldIncidence->uid());

    if (!oldIncidence) {
        // How can this happen ?
        kWarning() << "Couldn't find old incidence";
        Q_ASSERT(false);
        return;
    }

    if (newIncidence->instanceIdentifier() == oldIncidence->instanceIdentifier()) {
        kWarning() << "New uid=" << newIncidence->uid() << "; old uid=" << oldIncidence->uid()
                   << "; new recurrenceId="
                   << newIncidence->recurrenceId()
                   << "; old recurrenceId=" << oldIncidence->recurrenceId()
                   << "; new summary = " << newIncidence->summary()
                   << "; old summary = " << oldIncidence->summary()
                   << "; id = " << newItem.id();
        Q_ASSERT(false);   // The reason we're here in the first place
        return;
    }

    mItemIdByUid.remove(oldIncidence->instanceIdentifier());
    const QString oldUid = oldIncidence->uid();

    if (mParentUidToChildrenUid.contains(oldUid)) {
        Q_ASSERT(!mParentUidToChildrenUid.contains(newIdentifier));
        QStringList children = mParentUidToChildrenUid.value(oldUid);
        mParentUidToChildrenUid.insert(newIdentifier, children);
        mParentUidToChildrenUid.remove(oldUid);
    }

    // Update internal maps of the base class, MemoryCalendar
    q->setObserversEnabled(false);
    q->MemoryCalendar::deleteIncidence(oldIncidence);
    q->MemoryCalendar::addIncidence(newIncidence);

    newIncidence->setUid(oldUid); // We set and unset just to notify observers of a change.
    q->setObserversEnabled(true);
    newIncidence->setUid(newUid);
}

void CalendarBasePrivate::handleParentChanged(const KCalCore::Incidence::Ptr &newIncidence)
{
    Q_ASSERT(newIncidence);

    if (newIncidence->hasRecurrenceId()) {   // These ones don't/shouldn't have a parent
        return;
    }

    const QString originalParentUid = mUidToParent.value(newIncidence->uid());
    const QString newParentUid = newIncidence->relatedTo();

    if (originalParentUid == newParentUid) {
        return; // nothing changed
    }

    if (!originalParentUid.isEmpty()) {
        // Remove this child from it's old parent:
        Q_ASSERT(mParentUidToChildrenUid.contains(originalParentUid));
        mParentUidToChildrenUid[originalParentUid].removeAll(newIncidence->uid());
    }

    mUidToParent.remove(newIncidence->uid());

    if (!newParentUid.isEmpty()) {
        // Deliver this child to it's new parent:
        Q_ASSERT(!mParentUidToChildrenUid[newParentUid].contains(newIncidence->uid()));
        mParentUidToChildrenUid[newParentUid].append(newIncidence->uid());
        mUidToParent.insert(newIncidence->uid(), newParentUid);
    }
}

CalendarBase::CalendarBase(QObject *parent) : MemoryCalendar(KSystemTimeZones::local())
    , d_ptr(new CalendarBasePrivate(this))
{
    setParent(parent);
    setDeletionTracking(false);
}

CalendarBase::CalendarBase(CalendarBasePrivate *const dd,
                           QObject *parent) : MemoryCalendar(KSystemTimeZones::local())
    , d_ptr(dd)
{
    setParent(parent);
    setDeletionTracking(false);
}

CalendarBase::~CalendarBase()
{
}

Akonadi::Item CalendarBase::item(Akonadi::Item::Id id) const
{
    Q_D(const CalendarBase);
    Akonadi::Item i;
    if (d->mItemById.contains(id)) {
        i = d->mItemById[id];
    } else {
        kDebug() << "Can't find any item with id " << id;
    }
    return i;
}

Akonadi::Item CalendarBase::item(const QString &uid) const
{
    Q_D(const CalendarBase);
    Akonadi::Item i;

    if (uid.isEmpty())
        return i;

    if (d->mItemIdByUid.contains(uid)) {
        const Akonadi::Item::Id id = d->mItemIdByUid[uid];
        if (!d->mItemById.contains(id)) {
            kError() << "Item with id " << id << "(uid=" << uid << ") not found, but in uid map";
            Q_ASSERT_X(false, "CalendarBase::item", "not in mItemById");
        }
        i = d->mItemById[id];
    } else {
        kDebug() << "Can't find any incidence with uid " << uid;
    }
    return i;
}

Item CalendarBase::item(const Incidence::Ptr &incidence) const
{
    return incidence ? item(incidence->instanceIdentifier()) : Item();
}

Akonadi::Item::List CalendarBase::items() const
{
    Q_D(const CalendarBase);
    return d->mItemById.values();
}

Akonadi::Item::List CalendarBase::items(Akonadi::Collection::Id id) const
{
    Q_D(const CalendarBase);
    return d->mItemsByCollection.values(id);
}

Akonadi::Item::List CalendarBase::itemList(const KCalCore::Incidence::List &incidences) const
{
    Akonadi::Item::List items;

    foreach(const KCalCore::Incidence::Ptr &incidence, incidences) {
        if (incidence) {
            items << item(incidence->instanceIdentifier());
        } else {
            items << Akonadi::Item();
        }
    }

    return items;
}

KCalCore::Incidence::List CalendarBase::childIncidences(const Akonadi::Item::Id &parentId) const
{
    Q_D(const CalendarBase);
    KCalCore::Incidence::List childs;

    if (d->mItemById.contains(parentId)) {
        const Akonadi::Item item = d->mItemById.value(parentId);
        Q_ASSERT(item.isValid());
        KCalCore::Incidence::Ptr parent = CalendarUtils::incidence(item);

        if (parent) {
            childs = childIncidences(parent->uid());
        } else {
            Q_ASSERT(false);
        }
    }

    return childs;
}

KCalCore::Incidence::List CalendarBase::childIncidences(const QString &parentUid) const
{
    Q_D(const CalendarBase);
    KCalCore::Incidence::List children;
    const QStringList uids = d->mParentUidToChildrenUid.value(parentUid);
    Q_FOREACH(const QString &uid, uids) {
        Incidence::Ptr child = incidence(uid);
        if (child)
            children.append(child);
        else
            kWarning() << "Invalid child with uid " << uid;
    }
    return children;
}

Akonadi::Item::List CalendarBase::childItems(const Akonadi::Item::Id &parentId) const
{
    Q_D(const CalendarBase);
    Akonadi::Item::List childs;

    if (d->mItemById.contains(parentId)) {
        const Akonadi::Item item = d->mItemById.value(parentId);
        Q_ASSERT(item.isValid());
        KCalCore::Incidence::Ptr parent = CalendarUtils::incidence(item);

        if (parent) {
            childs = childItems(parent->uid());
        } else {
            Q_ASSERT(false);
        }
    }

    return childs;
}

Akonadi::Item::List CalendarBase::childItems(const QString &parentUid) const
{
    Q_D(const CalendarBase);
    Akonadi::Item::List children;
    const QStringList uids = d->mParentUidToChildrenUid.value(parentUid);
    Q_FOREACH(const QString &uid, uids) {
        Akonadi::Item child = item(uid);
        if (child.isValid() && child.hasPayload<KCalCore::Incidence::Ptr>())
            children.append(child);
        else
            kWarning() << "Invalid child with uid " << uid;
    }
    return children;
}

bool CalendarBase::addEvent(const KCalCore::Event::Ptr &event)
{
    return addIncidence(event);
}

bool CalendarBase::deleteEvent(const KCalCore::Event::Ptr &event)
{
    return deleteIncidence(event);
}

void CalendarBase::deleteAllEvents()
{
    Q_D(CalendarBase);
    d->deleteAllIncidencesOfType(Event::eventMimeType());
}

bool CalendarBase::addTodo(const KCalCore::Todo::Ptr &todo)
{
    return addIncidence(todo);
}

bool CalendarBase::deleteTodo(const KCalCore::Todo::Ptr &todo)
{
    return deleteIncidence(todo);
}

void CalendarBase::deleteAllTodos()
{
    Q_D(CalendarBase);
    d->deleteAllIncidencesOfType(Todo::todoMimeType());
}

bool CalendarBase::addJournal(const KCalCore::Journal::Ptr &journal)
{
    return addIncidence(journal);
}

bool CalendarBase::deleteJournal(const KCalCore::Journal::Ptr &journal)
{
    return deleteIncidence(journal);
}

void CalendarBase::deleteAllJournals()
{
    Q_D(CalendarBase);
    d->deleteAllIncidencesOfType(Journal::journalMimeType());
}

bool CalendarBase::addIncidence(const KCalCore::Incidence::Ptr &incidence)
{
    //TODO: Parent for dialogs
    Q_D(CalendarBase);

    // User canceled on the collection selection dialog
    if (batchAdding() && d->mBatchInsertionCancelled) {
        return false;
    }

    d->mLastCreationCancelled = false;

    Akonadi::Collection collection;

    if (batchAdding() && d->mCollectionForBatchInsertion.isValid()) {
        collection = d->mCollectionForBatchInsertion;
    }

    if (incidence->hasRecurrenceId() && !collection.isValid()) {
        // We are creating an exception, reuse the same collection that the main incidence uses
        Item mainItem = item(incidence->uid());
        if (mainItem.isValid()) {
            collection = Collection(mainItem.storageCollectionId());
        }
    }

    const int changeId = d->mIncidenceChanger->createIncidence(incidence, collection);

    if (batchAdding()) {
        const Akonadi::Collection lastCollection = d->mIncidenceChanger->lastCollectionUsed();
        if (changeId != -1 && !lastCollection.isValid()) {
            d->mBatchInsertionCancelled = true;
        } else if (lastCollection.isValid() && !d->mCollectionForBatchInsertion.isValid()) {
            d->mCollectionForBatchInsertion = d->mIncidenceChanger->lastCollectionUsed();
        }
    }

    return changeId != -1;
}

bool CalendarBase::deleteIncidence(const KCalCore::Incidence::Ptr &incidence)
{
    Q_D(CalendarBase);
    Q_ASSERT(incidence);
    Akonadi::Item item_ = item(incidence->instanceIdentifier());
    return -1 != d->mIncidenceChanger->deleteIncidence(item_);
}

bool CalendarBase::modifyIncidence(const KCalCore::Incidence::Ptr &newIncidence)
{
    Q_D(CalendarBase);
    Q_ASSERT(newIncidence);
    Akonadi::Item item_ = item(newIncidence->instanceIdentifier());
    item_.setPayload<KCalCore::Incidence::Ptr>(newIncidence);
    return -1 != d->mIncidenceChanger->modifyIncidence(item_);
}

void CalendarBase::setWeakPointer(const QWeakPointer<CalendarBase> &pointer)
{
    Q_D(CalendarBase);
    d->mWeakPointer = pointer;
}

QWeakPointer<CalendarBase> CalendarBase::weakPointer() const
{
    Q_D(const CalendarBase);
    return d->mWeakPointer;
}

IncidenceChanger* CalendarBase::incidenceChanger() const
{
    Q_D(const CalendarBase);
    return d->mIncidenceChanger;
}

void CalendarBase::startBatchAdding()
{
    KCalCore::MemoryCalendar::startBatchAdding();
}

void CalendarBase::endBatchAdding()
{
    Q_D(CalendarBase);
    d->mCollectionForBatchInsertion = Akonadi::Collection();
    d->mBatchInsertionCancelled = false;
    KCalCore::MemoryCalendar::endBatchAdding();
}

#include "moc_calendarbase.cpp"
#include "moc_calendarbase_p.cpp"
