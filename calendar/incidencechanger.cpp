/*
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
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
#include "incidencechanger.h"
#include "incidencechanger_p.h"
#include "mailscheduler_p.h"
#include "utils_p.h"

#include <akonadi/itemcreatejob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/transactionsequence.h>

#include <KJob>
#include <KLocalizedString>
#include <KGuiItem>
#include <KMessageBox>

#include <QBitArray>

using namespace Akonadi;
using namespace KCalCore;

#ifdef PLEASE_TEST_INVITATIONS
# define RUNNING_UNIT_TESTS true
#else
# define RUNNING_UNIT_TESTS false
#endif

ITIPHandlerHelper::Action actionFromStatus(ITIPHandlerHelper::SendResult result)
{
    //enum SendResult {
    //      Canceled,        /**< Sending was canceled by the user, meaning there are
    //                          local changes of which other attendees are not aware. */
    //      FailKeepUpdate,  /**< Sending failed, the changes to the incidence must be kept. */
    //      FailAbortUpdate, /**< Sending failed, the changes to the incidence must be undone. */
    //      NoSendingNeeded, /**< In some cases it is not needed to send an invitation
    //                          (e.g. when we are the only attendee) */
    //      Success
    switch (result) {
    case ITIPHandlerHelper::ResultCanceled:
        return ITIPHandlerHelper::ActionDontSendMessage;
    case ITIPHandlerHelper::ResultSuccess:
        return ITIPHandlerHelper::ActionSendMessage;
    default:
        return ITIPHandlerHelper::ActionAsk;
    }
}

namespace Akonadi {
// Does a queued emit, with QMetaObject::invokeMethod
static void emitCreateFinished(IncidenceChanger *changer,
                               int changeId,
                               const Akonadi::Item &item,
                               Akonadi::IncidenceChanger::ResultCode resultCode,
                               const QString &errorString)
{
    QMetaObject::invokeMethod(changer, "createFinished", Qt::QueuedConnection,
                              Q_ARG(int, changeId),
                              Q_ARG(Akonadi::Item, item),
                              Q_ARG(Akonadi::IncidenceChanger::ResultCode, resultCode),
                              Q_ARG(QString, errorString));
}

// Does a queued emit, with QMetaObject::invokeMethod
static void emitModifyFinished(IncidenceChanger *changer,
                               int changeId,
                               const Akonadi::Item &item,
                               IncidenceChanger::ResultCode resultCode,
                               const QString &errorString)
{
    QMetaObject::invokeMethod(changer, "modifyFinished", Qt::QueuedConnection,
                              Q_ARG(int, changeId),
                              Q_ARG(Akonadi::Item, item),
                              Q_ARG(Akonadi::IncidenceChanger::ResultCode, resultCode),
                              Q_ARG(QString, errorString));
}

// Does a queued emit, with QMetaObject::invokeMethod
static void emitDeleteFinished(IncidenceChanger *changer,
                               int changeId,
                               const QVector<Akonadi::Item::Id> &itemIdList,
                               IncidenceChanger::ResultCode resultCode,
                               const QString &errorString)
{
    QMetaObject::invokeMethod(changer, "deleteFinished", Qt::QueuedConnection,
                              Q_ARG(int, changeId),
                              Q_ARG(QVector<Akonadi::Item::Id>, itemIdList),
                              Q_ARG(Akonadi::IncidenceChanger::ResultCode, resultCode),
                              Q_ARG(QString, errorString));
}
}

class ConflictPreventerPrivate;
class ConflictPreventer {
    friend class ConflictPreventerPrivate;
public:
    static ConflictPreventer* self();

    // To avoid conflicts when the two modifications came from within the same application
    QHash<Akonadi::Item::Id, int> mLatestRevisionByItemId;
private:
    ConflictPreventer() {}
    ~ConflictPreventer() {}
};

class ConflictPreventerPrivate {
public:
    ConflictPreventer instance;
};

K_GLOBAL_STATIC(ConflictPreventerPrivate, sConflictPreventerPrivate)

ConflictPreventer* ConflictPreventer::self()
{
    return &sConflictPreventerPrivate->instance;
}

IncidenceChanger::Private::Private(bool enableHistory, ITIPHandlerComponentFactory *factory, IncidenceChanger *qq) : q(qq)
{
    mLatestChangeId = 0;
    mShowDialogsOnError = true;
    mFactory = factory ? factory : new ITIPHandlerComponentFactory(this);
    mHistory = enableHistory ? new History(this) : 0;
    mUseHistory = enableHistory;
    mDestinationPolicy = DestinationPolicyDefault;
    mRespectsCollectionRights = false;
    mGroupwareCommunication = false;
    mLatestAtomicOperationId = 0;
    mBatchOperationInProgress = false;
    mAutoAdjustRecurrence = true;
    m_collectionFetchJob = 0;
    m_invitationPolicy = InvitationPolicyAsk;

    qRegisterMetaType<QVector<Akonadi::Item::Id> >("QVector<Akonadi::Item::Id>");
    qRegisterMetaType<Akonadi::Item::Id>("Akonadi::Item::Id");
    qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
    qRegisterMetaType<Akonadi::IncidenceChanger::ResultCode>(
        "Akonadi::IncidenceChanger::ResultCode");
}

IncidenceChanger::Private::~Private()
{
    if (!mAtomicOperations.isEmpty() ||
            !mQueuedModifications.isEmpty() ||
            !mModificationsInProgress.isEmpty()) {
        kDebug() << "Normal if the application was being used. "
                 "But might indicate a memory leak if it wasn't";
    }
}

bool IncidenceChanger::Private::atomicOperationIsValid(uint atomicOperationId) const
{
    // Changes must be done between startAtomicOperation() and endAtomicOperation()
    return mAtomicOperations.contains(atomicOperationId) &&
           !mAtomicOperations[atomicOperationId]->m_endCalled;
}

bool IncidenceChanger::Private::hasRights(const Collection &collection,
        IncidenceChanger::ChangeType changeType) const
{
    bool result = false;
    switch (changeType) {
    case ChangeTypeCreate:
        result = collection.rights() & Akonadi::Collection::CanCreateItem;
        break;
    case ChangeTypeModify:
        result = collection.rights() & Akonadi::Collection::CanChangeItem;
        break;
    case ChangeTypeDelete:
        result = collection.rights() & Akonadi::Collection::CanDeleteItem;
        break;
    default:
        Q_ASSERT_X(false, "hasRights", "invalid type");
    }

    return !collection.isValid() || !mRespectsCollectionRights || result;
}

Akonadi::Job* IncidenceChanger::Private::parentJob(const Change::Ptr &change) const
{
    return (mBatchOperationInProgress && !change->queuedModification) ?
           mAtomicOperations[mLatestAtomicOperationId]->transaction() : 0;
}

void IncidenceChanger::Private::queueModification(Change::Ptr change)
{
    // If there's already a change queued we just discard it
    // and send the newer change, which already includes
    // previous modifications
    const Akonadi::Item::Id id = change->newItem.id();
    if (mQueuedModifications.contains(id)) {
        Change::Ptr toBeDiscarded = mQueuedModifications.take(id);
        toBeDiscarded->resultCode = ResultCodeModificationDiscarded;
        toBeDiscarded->completed  = true;
        mChangeById.remove(toBeDiscarded->id);
    }

    change->queuedModification = true;
    mQueuedModifications[id] = change;
}

void IncidenceChanger::Private::performNextModification(Akonadi::Item::Id id)
{
    mModificationsInProgress.remove(id);

    if (mQueuedModifications.contains(id)) {
        const Change::Ptr change = mQueuedModifications.take(id);
        performModification(change);
    }
}

void IncidenceChanger::Private::handleTransactionJobResult(KJob *job)
{
    //kDebug();
    TransactionSequence *transaction = qobject_cast<TransactionSequence*>(job);
    Q_ASSERT(transaction);
    Q_ASSERT(mAtomicOperationByTransaction.contains(transaction));

    const uint atomicOperationId = mAtomicOperationByTransaction.take(transaction);

    Q_ASSERT(mAtomicOperations.contains(atomicOperationId));
    AtomicOperation *operation = mAtomicOperations[atomicOperationId];
    Q_ASSERT(operation);
    Q_ASSERT(operation->m_id == atomicOperationId);
    if (job->error()) {
        if (!operation->rolledback())
            operation->setRolledback();
        kError() << "Transaction failed, everything was rolledback. "
                 << job->errorString();
    } else {
        Q_ASSERT(operation->m_endCalled);
        Q_ASSERT(!operation->pendingJobs());
    }

    if (!operation->pendingJobs() && operation->m_endCalled) {
        delete mAtomicOperations.take(atomicOperationId);
        mBatchOperationInProgress = false;
    } else {
        operation->m_transactionCompleted = true;
    }
}

void IncidenceChanger::Private::handleCreateJobResult(KJob *job)
{
    //kDebug();
    QString errorString;
    ResultCode resultCode = ResultCodeSuccess;

    Change::Ptr change = mChangeForJob.take(job);
    mChangeById.remove(change->id);

    const ItemCreateJob *j = qobject_cast<const ItemCreateJob*>(job);
    Q_ASSERT(j);
    Akonadi::Item item = j->item();

    QString description;
    if (change->atomicOperationId != 0) {
        AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
        a->m_numCompletedChanges++;
        change->completed = true;
        description = a->m_description;
    }

    if (j->error()) {
        item = change->newItem;
        resultCode = ResultCodeJobError;
        errorString = j->errorString();
        kError() << errorString;
        if (mShowDialogsOnError) {
            KMessageBox::sorry(change->parentWidget,
                               i18n("Error while trying to create calendar item. Error was: %1",
                                    errorString));
        }
    } else {
        Q_ASSERT(item.isValid());
        Q_ASSERT(item.hasPayload<KCalCore::Incidence::Ptr>());
        change->newItem = item;
        handleInvitationsAfterChange(change);
        // for user undo/redo
        if (change->recordToHistory) {
            mHistory->recordCreation(item, description, change->atomicOperationId);
        }
    }

    change->errorString = errorString;
    change->resultCode  = resultCode;
    // puff, change finally goes out of scope, and emits the incidenceCreated signal.
}

void IncidenceChanger::Private::handleDeleteJobResult(KJob *job)
{
    //kDebug();
    QString errorString;
    ResultCode resultCode = ResultCodeSuccess;

    Change::Ptr change = mChangeForJob.take(job);
    mChangeById.remove(change->id);

    const ItemDeleteJob *j = qobject_cast<const ItemDeleteJob*>(job);
    const Item::List items = j->deletedItems();

    QSharedPointer<DeletionChange> deletionChange = change.staticCast<DeletionChange>();

    foreach(const Akonadi::Item &item, items) {
        deletionChange->mItemIds.append(item.id());
    }
    QString description;
    if (change->atomicOperationId != 0) {
        AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
        a->m_numCompletedChanges++;
        change->completed = true;
        description = a->m_description;
    }
    if (j->error()) {
        resultCode = ResultCodeJobError;
        errorString = j->errorString();
        kError() << errorString;
        if (mShowDialogsOnError) {
            KMessageBox::sorry(change->parentWidget,
                               i18n("Error while trying to delete calendar item. Error was: %1",
                                    errorString));
        }

        foreach(const Item &item, items) {
            // Werent deleted due to error
            mDeletedItemIds.remove(mDeletedItemIds.indexOf(item.id()));
        }
    } else { // success
        if (change->recordToHistory) {
            Q_ASSERT(mHistory);
            mHistory->recordDeletions(items, description, change->atomicOperationId);
        }

        handleInvitationsAfterChange(change);
    }

    change->errorString = errorString;
    change->resultCode  = resultCode;
    // puff, change finally goes out of scope, and emits the incidenceDeleted signal.
}

void IncidenceChanger::Private::handleModifyJobResult(KJob *job)
{
    QString errorString;
    ResultCode resultCode = ResultCodeSuccess;
    Change::Ptr change = mChangeForJob.take(job);
    mChangeById.remove(change->id);

    const ItemModifyJob *j = qobject_cast<const ItemModifyJob*>(job);
    const Item item = j->item();
    Q_ASSERT(mDirtyFieldsByJob.contains(job));
    Q_ASSERT(item.hasPayload<KCalCore::Incidence::Ptr>());
    item.payload<KCalCore::Incidence::Ptr>()->setDirtyFields(mDirtyFieldsByJob.value(job));
    const QSet<KCalCore::IncidenceBase::Field> dirtyFields = mDirtyFieldsByJob.value(job);
    QString description;
    if (change->atomicOperationId != 0) {
        AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
        a->m_numCompletedChanges++;
        change->completed = true;
        description = a->m_description;
    }
    if (j->error()) {
        if (deleteAlreadyCalled(item.id())) {
            // User deleted the item almost at the same time he changed it. We could just return success
            // but the delete is probably already recorded to History, and that would make undo not work
            // in the proper order.
            resultCode = ResultCodeAlreadyDeleted;
            errorString = j->errorString();
            kWarning() << "Trying to change item " << item.id() << " while deletion is in progress.";
        } else {
            resultCode = ResultCodeJobError;
            errorString = j->errorString();
            kError() << errorString;
        }
        if (mShowDialogsOnError) {
            KMessageBox::sorry(change->parentWidget,
                               i18n("Error while trying to modify calendar item. Error was: %1",
                                    errorString));
        }
    } else { // success
        ConflictPreventer::self()->mLatestRevisionByItemId[item.id()] = item.revision();
        change->newItem = item;
        if (change->recordToHistory && !change->originalItems.isEmpty()) {
            Q_ASSERT(change->originalItems.count() == 1);
            mHistory->recordModification(change->originalItems.first(), item,
                                         description, change->atomicOperationId);
        }

        handleInvitationsAfterChange(change);
    }

    change->errorString = errorString;
    change->resultCode  = resultCode;
    // puff, change finally goes out of scope, and emits the incidenceModified signal.

    QMetaObject::invokeMethod(this, "performNextModification",
                              Qt::QueuedConnection,
                              Q_ARG(Akonadi::Item::Id, item.id()));
}

bool IncidenceChanger::Private::deleteAlreadyCalled(Akonadi::Item::Id id) const
{
    return mDeletedItemIds.contains(id);
}

bool IncidenceChanger::Private::handleInvitationsBeforeChange(const Change::Ptr &change)
{
    bool result = true;
    if (mGroupwareCommunication) {
        ITIPHandlerHelper handler(mFactory, change->parentWidget); // TODO make async

        if (m_invitationPolicy == InvitationPolicySend) {
            handler.setDefaultAction(ITIPHandlerHelper::ActionSendMessage);
        } else if (m_invitationPolicy == InvitationPolicyDontSend) {
            handler.setDefaultAction(ITIPHandlerHelper::ActionDontSendMessage);
        } else if (mInvitationStatusByAtomicOperation.contains(change->atomicOperationId)) {
            handler.setDefaultAction(actionFromStatus(mInvitationStatusByAtomicOperation.value(change->atomicOperationId)));
        }

        switch (change->type) {
        case IncidenceChanger::ChangeTypeCreate:
            // nothing needs to be done
            break;
        case IncidenceChanger::ChangeTypeDelete:
        {
            ITIPHandlerHelper::SendResult status;
            Q_ASSERT(!change->originalItems.isEmpty());
            foreach(const Akonadi::Item &item, change->originalItems) {
                Q_ASSERT(item.hasPayload<KCalCore::Incidence::Ptr>());
                Incidence::Ptr incidence = CalendarUtils::incidence(item);
                if (!incidence->supportsGroupwareCommunication()) {
                    continue;
                }
                // We only send CANCEL if we're the organizer.
                // If we're not, then we send REPLY with PartStat=Declined in handleInvitationsAfterChange()
                if (Akonadi::CalendarUtils::thatIsMe(incidence->organizer()->email())) {
                    status = handler.sendIncidenceDeletedMessage(KCalCore::iTIPCancel, incidence);
                    if (change->atomicOperationId) {
                        mInvitationStatusByAtomicOperation.insert(change->atomicOperationId, status);
                    }
                    result = status != ITIPHandlerHelper::ResultFailAbortUpdate;
                    //TODO: with some status we want to break immediately
                }
            }
        }
        break;
        case IncidenceChanger::ChangeTypeModify:
        {
            if (change->originalItems.isEmpty()) {
                break;
            }

            Q_ASSERT(change->originalItems.count() == 1);
            Incidence::Ptr oldIncidence = CalendarUtils::incidence(change->originalItems.first());
            Incidence::Ptr newIncidence = CalendarUtils::incidence(change->newItem);

            if (!oldIncidence->supportsGroupwareCommunication()) {
                break;
            }

            const bool weAreOrganizer = Akonadi::CalendarUtils::thatIsMe(newIncidence->organizer()->email());
            if (RUNNING_UNIT_TESTS && !weAreOrganizer) {
                // This is a bit of a workaround when running tests. I don't want to show the
                // "You're not organizer, do you want to modify event?" dialog in unit-tests, but want
                // to emulate a "yes" and a "no" press.
                if (m_invitationPolicy == InvitationPolicySend) {
                    return true;
                } else if (m_invitationPolicy == InvitationPolicyDontSend) {
                    return false;
                }
            }

            const bool modify = handler.handleIncidenceAboutToBeModified(newIncidence);
            if (modify) {
                break;
            }

            if (newIncidence->type() == oldIncidence->type()) {
                IncidenceBase *i1 = newIncidence.data();
                IncidenceBase *i2 = oldIncidence.data();
                *i1 = *i2;
            }
            result = false;
        }
        break;
        default:
            Q_ASSERT(false);
            result = false;
        }
    }
    return result;
}

bool IncidenceChanger::Private::handleInvitationsAfterChange(const Change::Ptr &change)
{
    if (change->useGroupwareCommunication) {
        ITIPHandlerHelper handler(mFactory, change->parentWidget);   // TODO make async

        const bool alwaysSend = m_invitationPolicy == InvitationPolicySend;
        const bool neverSend = m_invitationPolicy == InvitationPolicyDontSend;
        if (alwaysSend) {
            handler.setDefaultAction(ITIPHandlerHelper::ActionSendMessage);
        }

        if (neverSend) {
            handler.setDefaultAction(ITIPHandlerHelper::ActionDontSendMessage);
        }

        switch (change->type) {
        case IncidenceChanger::ChangeTypeCreate:
        {
            Incidence::Ptr incidence = CalendarUtils::incidence(change->newItem);
            if (incidence->supportsGroupwareCommunication()) {
                const ITIPHandlerHelper::SendResult status =
                    handler.sendIncidenceCreatedMessage(KCalCore::iTIPRequest, incidence);

                if (status == ITIPHandlerHelper::ResultFailAbortUpdate) {
                    kError() << "Sending invitations failed, but did not delete the incidence";
                }

                const uint atomicOperationId = change->atomicOperationId;
                if (atomicOperationId != 0) {
                    mInvitationStatusByAtomicOperation.insert(atomicOperationId, status);
                }
            }
        }
        break;
        case IncidenceChanger::ChangeTypeDelete:
        {
            Q_ASSERT(!change->originalItems.isEmpty());
            foreach(const Akonadi::Item &item, change->originalItems) {
                Q_ASSERT(item.hasPayload());
                Incidence::Ptr incidence = CalendarUtils::incidence(item);
                Q_ASSERT(incidence);
                if (!incidence->supportsGroupwareCommunication())
                    continue;

                if (!Akonadi::CalendarUtils::thatIsMe(incidence->organizer()->email())) {
                    const QStringList myEmails = Akonadi::CalendarUtils::allEmails();
                    bool notifyOrganizer = false;
                    for (QStringList::ConstIterator it = myEmails.begin(); it != myEmails.end(); ++it) {
                        const QString email = *it;
                        KCalCore::Attendee::Ptr me(incidence->attendeeByMail(email));
                        if (me) {
                            if (me->status() == KCalCore::Attendee::Accepted ||
                                    me->status() == KCalCore::Attendee::Delegated) {
                                notifyOrganizer = true;
                            }
                            KCalCore::Attendee::Ptr newMe(new KCalCore::Attendee(*me));
                            newMe->setStatus(KCalCore::Attendee::Declined);
                            incidence->clearAttendees();
                            incidence->addAttendee(newMe);
                            break;
                        }
                    }

                    if (notifyOrganizer) {
                        MailScheduler scheduler(mFactory, change->parentWidget); // TODO make async
                        scheduler.performTransaction(incidence, KCalCore::iTIPReply);
                    }
                }
            }
        }
        break;
        case IncidenceChanger::ChangeTypeModify:
        {
            if (change->originalItems.isEmpty()) {
                break;
            }

            Q_ASSERT(change->originalItems.count() == 1);
            Incidence::Ptr oldIncidence = CalendarUtils::incidence(change->originalItems.first());
            Incidence::Ptr newIncidence = CalendarUtils::incidence(change->newItem);

            if (!newIncidence->supportsGroupwareCommunication() ||
                    !Akonadi::CalendarUtils::thatIsMe(newIncidence->organizer()->email())) {
                // If we're not the organizer, the user already saw the "Do you really want to do this, incidence will become out of sync"
                break;
            }

            if (!neverSend && !alwaysSend && mInvitationStatusByAtomicOperation.contains(change->atomicOperationId)) {
                handler.setDefaultAction(actionFromStatus(mInvitationStatusByAtomicOperation.value(change->atomicOperationId)));
            }

            const bool attendeeStatusChanged = myAttendeeStatusChanged(newIncidence,
                                               oldIncidence,
                                               Akonadi::CalendarUtils::allEmails());

            ITIPHandlerHelper::SendResult status = handler.sendIncidenceModifiedMessage(KCalCore::iTIPRequest,
                                                   newIncidence,
                                                   attendeeStatusChanged);

            if (change->atomicOperationId != 0) {
                mInvitationStatusByAtomicOperation.insert(change->atomicOperationId, status);
            }
        }
        break;
        default:
            Q_ASSERT(false);
            return false;
        }
    }
    return true;
}

/** static */
bool IncidenceChanger::Private::myAttendeeStatusChanged(const Incidence::Ptr &newInc,
        const Incidence::Ptr &oldInc,
        const QStringList &myEmails)
{
    Q_ASSERT(newInc);
    Q_ASSERT(oldInc);
    const Attendee::Ptr oldMe = oldInc->attendeeByMails(myEmails);
    const Attendee::Ptr newMe = newInc->attendeeByMails(myEmails);

    return oldMe && newMe && oldMe->status() != newMe->status();
}

IncidenceChanger::IncidenceChanger(QObject *parent) : QObject(parent)
    , d(new Private(/**history=*/true, /*factory=*/0, this))
{
}


IncidenceChanger::IncidenceChanger(ITIPHandlerComponentFactory *factory, QObject *parent) : QObject(parent)
    , d(new Private(/**history=*/true, factory, this))
{
}

IncidenceChanger::IncidenceChanger(bool enableHistory, QObject *parent) : QObject(parent)
    , d(new Private(enableHistory, /*factory=*/0, this))
{
}

IncidenceChanger::~IncidenceChanger()
{
    delete d;
}

int IncidenceChanger::createIncidence(const Incidence::Ptr &incidence,
                                      const Collection &collection,
                                      QWidget *parent)
{
    //kDebug();
    if (!incidence) {
        kWarning() << "An invalid payload is not allowed.";
        d->cancelTransaction();
        return -1;
    }

    const uint atomicOperationId = d->mBatchOperationInProgress ? d->mLatestAtomicOperationId : 0;

    const Change::Ptr change(new CreationChange(this, ++d->mLatestChangeId,
                             atomicOperationId, parent));
    const int changeId = change->id;
    Q_ASSERT(!(d->mBatchOperationInProgress && !d->mAtomicOperations.contains(atomicOperationId)));
    if (d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback()) {
        const QString errorMessage = d->showErrorDialog(ResultCodeRolledback, parent);
        kWarning() << errorMessage;

        change->resultCode = ResultCodeRolledback;
        change->errorString = errorMessage;
        d->cleanupTransaction();
        return changeId;
    }

    Item item;
    item.setPayload<KCalCore::Incidence::Ptr>(incidence);
    item.setMimeType(incidence->mimeType());

    change->newItem = item;

    d->step1DetermineDestinationCollection(change, collection);

    return change->id;
}

int IncidenceChanger::deleteIncidence(const Item &item, QWidget *parent)
{
    Item::List list;
    list.append(item);

    return deleteIncidences(list, parent);
}

int IncidenceChanger::deleteIncidences(const Item::List &items, QWidget *parent)
{
    //kDebug();
    if (items.isEmpty()) {
        kError() << "Delete what?";
        d->cancelTransaction();
        return -1;
    }

    foreach(const Item &item, items) {
        if (!item.isValid()) {
            kError() << "Items must be valid!";
            d->cancelTransaction();
            return -1;
        }
    }

    const uint atomicOperationId = d->mBatchOperationInProgress ? d->mLatestAtomicOperationId : 0;
    const int changeId = ++d->mLatestChangeId;
    const Change::Ptr change(new DeletionChange(this, changeId, atomicOperationId, parent));

    foreach(const Item &item, items) {
        if (!d->hasRights(item.parentCollection(), ChangeTypeDelete)) {
            kWarning() << "Item " << item.id() << " can't be deleted due to ACL restrictions";
            const QString errorString = d->showErrorDialog(ResultCodePermissions, parent);
            change->resultCode = ResultCodePermissions;
            change->errorString = errorString;
            d->cancelTransaction();
            return changeId;
        }
    }

    if (!d->allowAtomicOperation(atomicOperationId, change)) {
        const QString errorString = d->showErrorDialog(ResultCodeDuplicateId, parent);
        change->resultCode = ResultCodeDuplicateId;
        change->errorString = errorString;
        kWarning() << errorString;
        d->cancelTransaction();
        return changeId;
    }

    Item::List itemsToDelete;
    foreach(const Item &item, items) {
        if (d->deleteAlreadyCalled(item.id())) {
            // IncidenceChanger::deleteIncidence() called twice, ignore this one.
            kDebug() << "Item " << item.id() << " already deleted or being deleted, skipping";
        } else {
            itemsToDelete.append(item);
        }
    }

    if (d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback()) {
        const QString errorMessage = d->showErrorDialog(ResultCodeRolledback, parent);
        change->resultCode = ResultCodeRolledback;
        change->errorString = errorMessage;
        kError() << errorMessage;
        d->cleanupTransaction();
        return changeId;
    }

    if (itemsToDelete.isEmpty()) {
        QVector<Akonadi::Item::Id> itemIdList;
        itemIdList.append(Item().id());
        kDebug() << "Items already deleted or being deleted, skipping";
        const QString errorMessage =
            i18n("That calendar item was already deleted, or currently being deleted.");
        // Queued emit because return must be executed first, otherwise caller won't know this workId
        change->resultCode = ResultCodeAlreadyDeleted;
        change->errorString = errorMessage;
        d->cancelTransaction();
        kWarning() << errorMessage;
        return changeId;
    }
    change->originalItems = itemsToDelete;
    d->handleInvitationsBeforeChange(change);

    ItemDeleteJob *deleteJob = new ItemDeleteJob(itemsToDelete, d->parentJob(change));
    d->mChangeForJob.insert(deleteJob, change);
    d->mChangeById.insert(changeId, change);

    if (d->mBatchOperationInProgress) {
        AtomicOperation *atomic = d->mAtomicOperations[atomicOperationId];
        Q_ASSERT(atomic);
        atomic->addChange(change);
    }

    foreach(const Item &item, itemsToDelete) {
        d->mDeletedItemIds << item.id();
    }

    // Do some cleanup
    if (d->mDeletedItemIds.count() > 100)
        d->mDeletedItemIds.remove(0, 50);

    // QueuedConnection because of possible sync exec calls.
    connect(deleteJob, SIGNAL(result(KJob*)),
            d, SLOT(handleDeleteJobResult(KJob*)), Qt::QueuedConnection);

    return changeId;
}

int IncidenceChanger::modifyIncidence(const Item &changedItem,
                                      const KCalCore::Incidence::Ptr &originalPayload,
                                      QWidget *parent)
{
    if (!changedItem.isValid() || !changedItem.hasPayload<Incidence::Ptr>()) {
        kWarning() << "An invalid item or payload is not allowed.";
        d->cancelTransaction();
        return -1;
    }

    if (!d->hasRights(changedItem.parentCollection(), ChangeTypeModify)) {
        kWarning() << "Item " << changedItem.id() << " can't be deleted due to ACL restrictions";
        const int changeId = ++d->mLatestChangeId;
        const QString errorString = d->showErrorDialog(ResultCodePermissions, parent);
        emitModifyFinished(this, changeId, changedItem, ResultCodePermissions, errorString);
        d->cancelTransaction();
        return changeId;
    }

    //TODO also update revision here instead of in the editor
    changedItem.payload<Incidence::Ptr>()->setLastModified(KDateTime::currentUtcDateTime());

    const uint atomicOperationId = d->mBatchOperationInProgress ? d->mLatestAtomicOperationId : 0;
    const int changeId = ++d->mLatestChangeId;
    ModificationChange *modificationChange = new ModificationChange(this, changeId,
            atomicOperationId, parent);
    Change::Ptr change(modificationChange);

    if (originalPayload) {
        Item originalItem(changedItem);
        originalItem.setPayload<KCalCore::Incidence::Ptr>(originalPayload);
        modificationChange->originalItems << originalItem;
    }

    modificationChange->newItem = changedItem;
    d->mChangeById.insert(changeId, change);

    if (!d->allowAtomicOperation(atomicOperationId, change)) {
        const QString errorString = d->showErrorDialog(ResultCodeDuplicateId, parent);
        change->resultCode = ResultCodeDuplicateId;
        change->errorString = errorString;
        d->cancelTransaction();
        kWarning() << "Atomic operation now allowed";
        return changeId;
    }

    if (d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback()) {
        const QString errorMessage = d->showErrorDialog(ResultCodeRolledback, parent);
        kError() << errorMessage;
        d->cleanupTransaction();
        emitModifyFinished(this, changeId, changedItem, ResultCodeRolledback, errorMessage);
    } else {
        d->adjustRecurrence(originalPayload, CalendarUtils::incidence(modificationChange->newItem));
        d->performModification(change);
    }

    return changeId;
}

void IncidenceChanger::Private::performModification(Change::Ptr change)
{
    const Item::Id id = change->newItem.id();
    Akonadi::Item &newItem = change->newItem;
    Q_ASSERT(newItem.isValid());
    Q_ASSERT(newItem.hasPayload<Incidence::Ptr>());

    const int changeId = change->id;

    if (deleteAlreadyCalled(id)) {
        // IncidenceChanger::deleteIncidence() called twice, ignore this one.
        kDebug() << "Item " << id << " already deleted or being deleted, skipping";

        // Queued emit because return must be executed first, otherwise caller won't know this workId
        emitModifyFinished(q, change->id, newItem, ResultCodeAlreadyDeleted,
                           i18n("That calendar item was already deleted, or currently being deleted."));
        return;
    }

    const uint atomicOperationId = change->atomicOperationId;
    const bool hasAtomicOperationId = atomicOperationId != 0;
    if (hasAtomicOperationId &&
            mAtomicOperations[atomicOperationId]->rolledback()) {
        const QString errorMessage = showErrorDialog(ResultCodeRolledback, 0);
        kError() << errorMessage;
        emitModifyFinished(q, changeId, newItem, ResultCodeRolledback, errorMessage);
        return;
    }

    const bool userCancelled = !handleInvitationsBeforeChange(change);
    if (userCancelled) {
        // User got a "You're not the organizer, do you really want to send" dialog, and said "no"
        kDebug() << "User cancelled, giving up";
        emitModifyFinished(q, changeId, newItem, ResultCodeUserCanceled, QString());
        return;
    }

    QHash<Akonadi::Item::Id, int> &latestRevisionByItemId =
        ConflictPreventer::self()->mLatestRevisionByItemId;
    if (latestRevisionByItemId.contains(id) &&
            latestRevisionByItemId[id] > newItem.revision()) {
        /* When a ItemModifyJob ends, the application can still modify the old items if the user
         * is quick because the ETM wasn't updated yet, and we'll get a STORE error, because
         * we are not modifying the latest revision.
         *
         * When a job ends, we keep the new revision in mLatestRevisionByItemId
         * so we can update the item's revision
         */
        newItem.setRevision(latestRevisionByItemId[id]);
    }

    Incidence::Ptr incidence = CalendarUtils::incidence(newItem);
    {   // increment revision ( KCalCore revision, not akonadi )
        const int revision = incidence->revision();
        incidence->setRevision(revision + 1);
    }

    // Dav Fix
    // Don't write back remote revision since we can't make sure it is the current one
    newItem.setRemoteRevision(QString());

    if (mModificationsInProgress.contains(newItem.id())) {
        // There's already a ItemModifyJob running for this item ID
        // Let's wait for it to end.
        queueModification(change);
    } else {
        ItemModifyJob *modifyJob = new ItemModifyJob(newItem, parentJob(change));
        mChangeForJob.insert(modifyJob, change);
        mDirtyFieldsByJob.insert(modifyJob, incidence->dirtyFields());

        if (hasAtomicOperationId) {
            AtomicOperation *atomic = mAtomicOperations[atomicOperationId];
            Q_ASSERT(atomic);
            atomic->addChange(change);
        }

        mModificationsInProgress[newItem.id()] = change;
        // QueuedConnection because of possible sync exec calls.
        connect(modifyJob, SIGNAL(result(KJob*)),
                SLOT(handleModifyJobResult(KJob*)), Qt::QueuedConnection);
    }
}

void IncidenceChanger::startAtomicOperation(const QString &operationDescription)
{
    if (d->mBatchOperationInProgress) {
        kDebug() << "An atomic operation is already in progress.";
        return;
    }

    ++d->mLatestAtomicOperationId;
    d->mBatchOperationInProgress = true;

    AtomicOperation *atomicOperation = new AtomicOperation(d, d->mLatestAtomicOperationId);
    atomicOperation->m_description = operationDescription;
    d->mAtomicOperations.insert(d->mLatestAtomicOperationId, atomicOperation);
}

void IncidenceChanger::endAtomicOperation()
{
    if (!d->mBatchOperationInProgress) {
        kDebug() << "No atomic operation is in progress.";
        return;
    }

    Q_ASSERT_X(d->mLatestAtomicOperationId != 0,
               "IncidenceChanger::endAtomicOperation()",
               "Call startAtomicOperation() first.");

    Q_ASSERT(d->mAtomicOperations.contains(d->mLatestAtomicOperationId));
    AtomicOperation *atomicOperation = d->mAtomicOperations[d->mLatestAtomicOperationId];
    Q_ASSERT(atomicOperation);
    atomicOperation->m_endCalled = true;

    const bool allJobsCompleted = !atomicOperation->pendingJobs();

    if (allJobsCompleted && atomicOperation->rolledback() &&
            atomicOperation->m_transactionCompleted) {
        // The transaction job already completed, we can cleanup:
        delete d->mAtomicOperations.take(d->mLatestAtomicOperationId);
        d->mBatchOperationInProgress = false;
    }/* else if ( allJobsCompleted ) {
    Q_ASSERT( atomicOperation->transaction );
    atomicOperation->transaction->commit(); we using autocommit now
  }*/
}

void IncidenceChanger::setShowDialogsOnError(bool enable)
{
    d->mShowDialogsOnError = enable;
}

bool IncidenceChanger::showDialogsOnError() const
{
    return d->mShowDialogsOnError;
}

void IncidenceChanger::setRespectsCollectionRights(bool respects)
{
    d->mRespectsCollectionRights = respects;
}

bool IncidenceChanger::respectsCollectionRights() const
{
    return d->mRespectsCollectionRights;
}

void IncidenceChanger::setDestinationPolicy(IncidenceChanger::DestinationPolicy destinationPolicy)
{
    d->mDestinationPolicy = destinationPolicy;
}

IncidenceChanger::DestinationPolicy IncidenceChanger::destinationPolicy() const
{
    return d->mDestinationPolicy;
}

void IncidenceChanger::setDefaultCollection(const Akonadi::Collection &collection)
{
    d->mDefaultCollection = collection;
}

Collection IncidenceChanger::defaultCollection() const
{
    return d->mDefaultCollection;
}

bool IncidenceChanger::historyEnabled() const
{
    return d->mUseHistory;
}

void IncidenceChanger::setHistoryEnabled(bool enable)
{
    if (d->mUseHistory != enable) {
        d->mUseHistory = enable;
        if (enable && !d->mHistory)
            d->mHistory = new History(d);
    }
}

History* IncidenceChanger::history() const
{
    return d->mHistory;
}

bool IncidenceChanger::deletedRecently(Akonadi::Item::Id id) const
{
    return d->deleteAlreadyCalled(id);
}

void IncidenceChanger::setGroupwareCommunication(bool enabled)
{
    d->mGroupwareCommunication = enabled;
}

bool IncidenceChanger::groupwareCommunication() const
{
    return d->mGroupwareCommunication;
}

void IncidenceChanger::setAutoAdjustRecurrence(bool enable)
{
    d->mAutoAdjustRecurrence = enable;
}

bool IncidenceChanger::autoAdjustRecurrence() const
{
    return d->mAutoAdjustRecurrence;
}

void IncidenceChanger::setInvitationPolicy(IncidenceChanger::InvitationPolicy policy)
{
    d->m_invitationPolicy = policy;
}

IncidenceChanger::InvitationPolicy IncidenceChanger::invitationPolicy() const
{
    return d->m_invitationPolicy;
}

Akonadi::Collection IncidenceChanger::lastCollectionUsed() const
{
    return d->mLastCollectionUsed;
}

QString IncidenceChanger::Private::showErrorDialog(IncidenceChanger::ResultCode resultCode,
        QWidget *parent)
{
    QString errorString;
    switch (resultCode) {
    case IncidenceChanger::ResultCodePermissions:
        errorString = i18n("Operation can not be performed due to ACL restrictions");
        break;
    case IncidenceChanger::ResultCodeInvalidUserCollection:
        errorString = i18n("The chosen collection is invalid");
        break;
    case IncidenceChanger::ResultCodeInvalidDefaultCollection:
        errorString = i18n("Default collection is invalid or doesn't have proper ACLs"
                           " and DestinationPolicyNeverAsk was used");
        break;
    case IncidenceChanger::ResultCodeDuplicateId:
        errorString = i18n("Duplicate item id in a group operation");
        break;
    case IncidenceChanger::ResultCodeRolledback:
        errorString = i18n("One change belonging to a group of changes failed. "
                           "All changes are being rolled back.");
        break;
    default:
        Q_ASSERT(false);
        return QString(i18n("Unknown error"));
    }

    if (mShowDialogsOnError) {
        KMessageBox::sorry(parent, errorString);
    }

    return errorString;
}

void IncidenceChanger::Private::adjustRecurrence(const KCalCore::Incidence::Ptr &originalIncidence,
        const KCalCore::Incidence::Ptr &incidence)
{
    if (!originalIncidence || !incidence->recurs() || incidence->hasRecurrenceId() || !mAutoAdjustRecurrence
            || !incidence->dirtyFields().contains(KCalCore::Incidence::FieldDtStart)) {
        return;
    }

    const QDate originalDate = originalIncidence->dtStart().date();
    const QDate newStartDate = incidence->dtStart().date();

    if (!originalDate.isValid() || !newStartDate.isValid() || originalDate == newStartDate)
        return;

    KCalCore::Recurrence *recurrence = incidence->recurrence();
    switch (recurrence->recurrenceType()) {
    case KCalCore::Recurrence::rWeekly: {
        QBitArray days = recurrence->days();
        const int oldIndex = originalDate.dayOfWeek()-1; // QDate returns [1-7];
        const int newIndex = newStartDate.dayOfWeek()-1;
        if (oldIndex != newIndex) {
            days.clearBit(oldIndex);
            days.setBit(newIndex);
            recurrence->setWeekly(recurrence->frequency(), days);
        }
    }
    default:
        break;   // Other types not implemented
    }

    // Now fix cases where dtstart would be bigger than the recurrence end rendering it impossible for a view to show it:
    // To retrieve the recurrence end don't trust Recurrence::endDt() since it returns dtStart if the rrule's end is < than dtstart,
    // it seems someone made Recurrence::endDt() more robust, but getNextOccurrences() still craps out. So lets fix it here
    // there's no reason to write bogus ical to disk.
    const QDate recurrenceEndDate = recurrence->defaultRRule() ? recurrence->defaultRRule()->endDt().date() : QDate();
    if (recurrenceEndDate.isValid() && recurrenceEndDate < newStartDate) {
        recurrence->setEndDate(newStartDate);
    }
}

void IncidenceChanger::Private::cancelTransaction()
{
    if (mBatchOperationInProgress) {
        mAtomicOperations[mLatestAtomicOperationId]->setRolledback();
    }
}

void IncidenceChanger::Private::cleanupTransaction()
{
    Q_ASSERT(mAtomicOperations.contains(mLatestAtomicOperationId));
    AtomicOperation *operation = mAtomicOperations[mLatestAtomicOperationId];
    Q_ASSERT(operation);
    Q_ASSERT(operation->rolledback());
    if (!operation->pendingJobs() && operation->m_endCalled && operation->m_transactionCompleted) {
        delete mAtomicOperations.take(mLatestAtomicOperationId);
        mBatchOperationInProgress = false;
    }
}

bool IncidenceChanger::Private::allowAtomicOperation(int atomicOperationId,
        const Change::Ptr &change) const
{
    bool allow = true;
    if (atomicOperationId > 0) {
        Q_ASSERT(mAtomicOperations.contains(atomicOperationId));
        AtomicOperation *operation = mAtomicOperations.value(atomicOperationId);

        if (change->type == ChangeTypeCreate) {
            allow = true;
        } else if (change->type == ChangeTypeModify) {
            allow = !operation->m_itemIdsInOperation.contains(change->newItem.id());
        } else if (change->type == ChangeTypeDelete) {
            DeletionChange::Ptr deletion = change.staticCast<DeletionChange>();
            foreach(Akonadi::Item::Id id, deletion->mItemIds) {
                if (operation->m_itemIdsInOperation.contains(id)) {
                    allow = false;
                    break;
                }
            }
        }
    }

    if (!allow) {
        kWarning() << "Each change belonging to a group operation"
                   << "must have a different Akonadi::Item::Id";
    }

    return allow;
}

/**reimp*/
void ModificationChange::emitCompletionSignal()
{
    emitModifyFinished(changer, id, newItem, resultCode, errorString);
}

/**reimp*/
void CreationChange::emitCompletionSignal()
{
    // Does a queued emit, with QMetaObject::invokeMethod
    emitCreateFinished(changer, id, newItem, resultCode, errorString);
}

/**reimp*/
void DeletionChange::emitCompletionSignal()
{
    emitDeleteFinished(changer, id, mItemIds, resultCode, errorString);
}

/**
Lost code from KDE 4.4 that was never called/used with incidenceeditors-ng.

      Attendees were removed from this incidence. Only the removed attendees
      are present in the incidence, so we just need to send a cancel messages
      to all attendees groupware messages are enabled at all.

void IncidenceChanger::cancelAttendees( const Akonadi::Item &aitem )
{
  const KCalCore::Incidence::Ptr incidence = CalendarSupport::incidence( aitem );
  Q_ASSERT( incidence );
  if ( KCalPrefs::instance()->mUseGroupwareCommunication ) {
    if ( KMessageBox::questionYesNo(
           0,
           i18n( "Some attendees were removed from the incidence. "
                 "Shall cancel messages be sent to these attendees?" ),
           i18n( "Attendees Removed" ), KGuiItem( i18n( "Send Messages" ) ),
           KGuiItem( i18n( "Do Not Send" ) ) ) == KMessageBox::Yes ) {
      // don't use Akonadi::Groupware::sendICalMessage here, because that asks just
      // a very general question "Other people are involved, send message to
      // them?", which isn't helpful at all in this situation. Afterwards, it
      // would only call the Akonadi::MailScheduler::performTransaction, so do this
      // manually.
      CalendarSupport::MailScheduler scheduler(
        static_cast<CalendarSupport::Calendar*>(d->mCalendar) );
      scheduler.performTransaction( incidence, KCalCore::iTIPCancel );
    }
  }
}

*/

AtomicOperation::AtomicOperation(IncidenceChanger::Private *icp,
                                 uint ident) : m_id(ident)
    , m_endCalled(false)
    , m_numCompletedChanges(0)
    , m_transactionCompleted(false)
    , m_wasRolledback(false)
    , m_transaction(0)
    , m_incidenceChangerPrivate(icp)

{
    Q_ASSERT(m_id != 0);
}

Akonadi::TransactionSequence *AtomicOperation::transaction()
{
    if (!m_transaction) {
        m_transaction = new Akonadi::TransactionSequence;
        m_transaction->setAutomaticCommittingEnabled(true);

        m_incidenceChangerPrivate->mAtomicOperationByTransaction.insert(m_transaction, m_id);

        QObject::connect(m_transaction, SIGNAL(result(KJob*)),
                         m_incidenceChangerPrivate, SLOT(handleTransactionJobResult(KJob*)));
    }

    return m_transaction;
}
