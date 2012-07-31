/*
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (C) 2010-2011 Sérgio Martins <iamsergio@gmail.com>

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

#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/TransactionSequence>
#include <Akonadi/CollectionDialog>

#include <KJob>
#include <KLocale>
#include <KGuiItem>
#include <KMessageBox>
#include <KStandardGuiItem>

using namespace Akonadi;
using namespace KCalCore;

InvitationHandler::Action actionFromStatus( InvitationHandler::SendResult result )
{
  //enum SendResult {
  //      Canceled,        /**< Sending was canceled by the user, meaning there are
  //                          local changes of which other attendees are not aware. */
  //      FailKeepUpdate,  /**< Sending failed, the changes to the incidence must be kept. */
  //      FailAbortUpdate, /**< Sending failed, the changes to the incidence must be undone. */
  //      NoSendingNeeded, /**< In some cases it is not needed to send an invitation
  //                          (e.g. when we are the only attendee) */
  //      Success
  switch ( result ) {
  case InvitationHandler::ResultCanceled:
    return InvitationHandler::ActionDontSendMessage;
  case InvitationHandler::ResultSuccess:
    return InvitationHandler::ActionSendMessage;
  default:
    return InvitationHandler::ActionAsk;
  }
}

namespace Akonadi {
  static Akonadi::Collection selectCollection( QWidget *parent,
                                               int &dialogCode,
                                               const QStringList &mimeTypes,
                                               const Akonadi::Collection &defCollection )
  {
    QPointer<Akonadi::CollectionDialog> dlg( new Akonadi::CollectionDialog( parent ) );

    kDebug() << "selecting collections with mimeType in " << mimeTypes;

    dlg->setMimeTypeFilter( mimeTypes );
    dlg->setAccessRightsFilter( Akonadi::Collection::CanCreateItem );
    if ( defCollection.isValid() ) {
      dlg->setDefaultCollection( defCollection );
    }
    Akonadi::Collection collection;

    // FIXME: don't use exec.
    dialogCode = dlg->exec();
    if ( dialogCode == QDialog::Accepted ) {
      collection = dlg->selectedCollection();

      if ( !collection.isValid() ) {
        kWarning() <<"An invalid collection was selected!";
      }
    }
    delete dlg;

    return collection;
  }

  // Does a queued emit, with QMetaObject::invokeMethod
  static void emitCreateFinished( IncidenceChanger *changer,
                                  int changeId,
                                  const Akonadi::Item &item,
                                  Akonadi::IncidenceChanger::ResultCode resultCode,
                                  const QString &errorString )
  {
    QMetaObject::invokeMethod( changer, "createFinished", Qt::QueuedConnection,
                              Q_ARG( int, changeId ),
                              Q_ARG( Akonadi::Item, item ),
                              Q_ARG( Akonadi::IncidenceChanger::ResultCode, resultCode ),
                              Q_ARG( QString, errorString ) );
  }

  // Does a queued emit, with QMetaObject::invokeMethod
  static void emitModifyFinished( IncidenceChanger *changer,
                                  int changeId,
                                  const Akonadi::Item &item,
                                  IncidenceChanger::ResultCode resultCode,
                                  const QString &errorString )
  {
    QMetaObject::invokeMethod( changer, "modifyFinished", Qt::QueuedConnection,
                              Q_ARG( int, changeId ),
                              Q_ARG( Akonadi::Item, item ),
                              Q_ARG( Akonadi::IncidenceChanger::ResultCode, resultCode ),
                              Q_ARG( QString, errorString ) );
  }

  // Does a queued emit, with QMetaObject::invokeMethod
  static void emitDeleteFinished( IncidenceChanger *changer,
                                  int changeId,
                                  const QVector<Akonadi::Item::Id> &itemIdList,
                                  IncidenceChanger::ResultCode resultCode,
                                  const QString &errorString )
  {
    QMetaObject::invokeMethod( changer, "deleteFinished", Qt::QueuedConnection,
                               Q_ARG( int, changeId ),
                               Q_ARG( QVector<Akonadi::Item::Id>, itemIdList ),
                               Q_ARG( Akonadi::IncidenceChanger::ResultCode, resultCode ),
                               Q_ARG( QString, errorString ) );
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

K_GLOBAL_STATIC( ConflictPreventerPrivate, sConflictPreventerPrivate );

ConflictPreventer* ConflictPreventer::self()
{
  return &sConflictPreventerPrivate->instance;
}

IncidenceChanger::Private::Private( bool enableHistory, IncidenceChanger *qq ) : q( qq )
{
  mLatestChangeId = 0;
  mShowDialogsOnError = true;
  mHistory = enableHistory ? new History( this ) : 0;
  mUseHistory = enableHistory;
  mDestinationPolicy = DestinationPolicyDefault;
  mRespectsCollectionRights = false;
  mGroupwareCommunication = false;
  mLatestAtomicOperationId = 0;
  mBatchOperationInProgress = false;

  qRegisterMetaType<QVector<Akonadi::Item::Id> >( "QVector<Akonadi::Item::Id>" );
  qRegisterMetaType<Akonadi::Item::Id>( "Akonadi::Item::Id" );

  qRegisterMetaType<Akonadi::IncidenceChanger::ResultCode>(
    "Akonadi::IncidenceChanger::ResultCode" );
}

IncidenceChanger::Private::~Private()
{
  if ( !mAtomicOperations.isEmpty() ||
       !mQueuedModifications.isEmpty() ||
       !mModificationsInProgress.isEmpty() ) {
    kDebug() << "Normal if the application was being used. "
                "But might indicate a memory leak if it wasn't";
  }
}

bool IncidenceChanger::Private::atomicOperationIsValid( uint atomicOperationId ) const
{
  // Changes must be done between startAtomicOperation() and endAtomicOperation()
  return mAtomicOperations.contains( atomicOperationId ) &&
         !mAtomicOperations[atomicOperationId]->endCalled;
}

bool IncidenceChanger::Private::hasRights( const Collection &collection,
                                            IncidenceChanger::ChangeType changeType ) const
{
  bool result = false;
  switch( changeType ) {
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
      Q_ASSERT_X( false, "hasRights", "invalid type" );
  }

  return !collection.isValid() || !mRespectsCollectionRights || result;
}

Akonadi::Job* IncidenceChanger::Private::parentJob( const Change::Ptr &change ) const
{
  return (mBatchOperationInProgress && !change->queuedModification) ?
                                       mAtomicOperations[mLatestAtomicOperationId]->transaction : 0;
}

void IncidenceChanger::Private::queueModification( Change::Ptr change )
{
  // If there's already a change queued we just discard it
  // and send the newer change, which already includes
  // previous modifications
  const Akonadi::Item::Id id = change->newItem.id();
  if ( mQueuedModifications.contains( id ) ) {
    Change::Ptr toBeDiscarded = mQueuedModifications.take( id );
    toBeDiscarded->resultCode = ResultCodeModificationDiscarded;
    toBeDiscarded->completed  = true;
    mChangeById.remove( toBeDiscarded->id );
  }

  change->queuedModification = true;
  mQueuedModifications[id] = change;
}

void IncidenceChanger::Private::performNextModification( Akonadi::Item::Id id )
{
  mModificationsInProgress.remove( id );

  if ( mQueuedModifications.contains( id ) ) {
    const Change::Ptr change = mQueuedModifications.take( id );
    performModification( change );
  }
}

void IncidenceChanger::Private::handleTransactionJobResult( KJob *job )
{
  //kDebug();
  TransactionSequence *transaction = qobject_cast<TransactionSequence*>( job );
  Q_ASSERT( transaction );
  Q_ASSERT( mAtomicOperationByTransaction.contains( transaction ) );

  const uint atomicOperationId = mAtomicOperationByTransaction.take( transaction );

  Q_ASSERT( mAtomicOperations.contains(atomicOperationId) );
  AtomicOperation *operation = mAtomicOperations[atomicOperationId];
  Q_ASSERT( operation );
  Q_ASSERT( operation->id == atomicOperationId );
  if ( job->error() ) {
    if ( !operation->rolledback() )
      operation->setRolledback();
    kError() << "Transaction failed, everything was rolledback. "
             << job->errorString();
  } else {
    Q_ASSERT( operation->endCalled );
    Q_ASSERT( !operation->pendingJobs() );
  }

  if ( !operation->pendingJobs() && operation->endCalled ) {
    delete mAtomicOperations.take( atomicOperationId );
    mBatchOperationInProgress = false;
  } else {
    operation->transactionCompleted = true;
  }
}

void IncidenceChanger::Private::handleCreateJobResult( KJob *job )
{
  //kDebug();
  QString errorString;
  ResultCode resultCode = ResultCodeSuccess;

  Change::Ptr change = mChangeForJob.take( job );
  mChangeById.remove( change->id );

  const ItemCreateJob *j = qobject_cast<const ItemCreateJob*>( job );
  Q_ASSERT( j );
  Akonadi::Item item = j->item();

  QString description;
  if ( change->atomicOperationId != 0 ) {
    AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
    a->numCompletedChanges++;
    change->completed = true;
    description = a->description;
  }

  if ( j->error() ) {
    item = change->newItem;
    resultCode = ResultCodeJobError;
    errorString = j->errorString();
    kError() << errorString;
    if ( mShowDialogsOnError ) {
      KMessageBox::sorry( change->parentWidget,
                          i18n( "Error while trying to create calendar item. Error was: %1",
                                errorString ) );
    }
  } else {
    Q_ASSERT( item.isValid() );
    Q_ASSERT( item.hasPayload<KCalCore::Incidence::Ptr>() );
    change->newItem = item;
    // for user undo/redo
    if ( change->recordToHistory ) {
      mHistory->recordCreation( item, description, change->atomicOperationId );
    }
  }

  change->errorString = errorString;
  change->resultCode  = resultCode;
  // puff, change finally goes out of scope, and emits the incidenceCreated signal.
}

void IncidenceChanger::Private::handleDeleteJobResult( KJob *job )
{
  //kDebug();
  QString errorString;
  ResultCode resultCode = ResultCodeSuccess;

  Change::Ptr change = mChangeForJob.take( job );
  mChangeById.remove( change->id );

  const ItemDeleteJob *j = qobject_cast<const ItemDeleteJob*>( job );
  const Item::List items = j->deletedItems();

  QSharedPointer<DeletionChange> deletionChange = change.staticCast<DeletionChange>();

  foreach( const Akonadi::Item &item, items ) {
    deletionChange->mItemIds.append( item.id() );
  }
  QString description;
  if ( change->atomicOperationId != 0 ) {
    AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
    a->numCompletedChanges++;
    change->completed = true;
    description = a->description;
  }
  if ( j->error() ) {
    resultCode = ResultCodeJobError;
    errorString = j->errorString();
    kError() << errorString;
    if ( mShowDialogsOnError ) {
      KMessageBox::sorry( change->parentWidget,
                          i18n( "Error while trying to delete calendar item. Error was: %1",
                          errorString ) );
    }

    foreach( const Item &item, items ) {
      // Werent deleted due to error
      //TODO: this really needed? will eat some memory
      mDeletedItemIds.remove( item.id() );
    }
  } else { // success
    foreach( const Item &item, items ) {
      ConflictPreventer::self()->mLatestRevisionByItemId.remove( item.id() );
      if ( change->recordToHistory && item.hasPayload<KCalCore::Incidence::Ptr>() ) {
        //TODO: check return value
        //TODO: make History support a list of items
        mHistory->recordDeletion( item, description, change->atomicOperationId );
      }
    }
  }

  change->errorString = errorString;
  change->resultCode  = resultCode;
  // puff, change finally goes out of scope, and emits the incidenceDeleted signal.
}

void IncidenceChanger::Private::handleModifyJobResult( KJob *job )
{
  QString errorString;
  ResultCode resultCode = ResultCodeSuccess;
  Change::Ptr change = mChangeForJob.take( job );
  mChangeById.remove( change->id );

  const ItemModifyJob *j = qobject_cast<const ItemModifyJob*>( job );
  const Item item = j->item();
  Q_ASSERT( mDirtyFieldsByJob.contains( job ) );
  item.payload<KCalCore::Incidence::Ptr>()->setDirtyFields( mDirtyFieldsByJob.value( job ) );
  const QSet<KCalCore::IncidenceBase::Field> dirtyFields = mDirtyFieldsByJob.value( job );
  QString description;
  if ( change->atomicOperationId != 0 ) {
    AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
    a->numCompletedChanges++;
    change->completed = true;
    description = a->description;
  }
  if ( j->error() ) {
    if ( deleteAlreadyCalled( item.id() ) ) {
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
    if ( mShowDialogsOnError ) {
      KMessageBox::sorry( change->parentWidget,
                          i18n( "Error while trying to modify calendar item. Error was: %1",
                          errorString ) );
    }
  } else { // success
    ConflictPreventer::self()->mLatestRevisionByItemId[item.id()] = item.revision();
    change->newItem = item;
    if ( change->recordToHistory && change->originalItem.isValid() ) {
      mHistory->recordModification( change->originalItem, item,
                                    description, change->atomicOperationId );
    }
  }

  change->errorString = errorString;
  change->resultCode  = resultCode;
  // puff, change finally goes out of scope, and emits the incidenceModified signal.

  QMetaObject::invokeMethod( this, "performNextModification",
                             Qt::QueuedConnection,
                             Q_ARG( Akonadi::Item::Id, item.id() ) );
}

bool IncidenceChanger::Private::deleteAlreadyCalled( Akonadi::Item::Id id ) const
{
  return mDeletedItemIds.contains( id );
}

bool IncidenceChanger::Private::handleInvitationsBeforeChange( const Change::Ptr &change )
{
  bool result = true;
  if ( mGroupwareCommunication ) {
    InvitationHandler handler( FetchJobCalendar::Ptr(), change->parentWidget );  // TODO make async
    if ( mInvitationStatusByAtomicOperation.contains( change->atomicOperationId ) ) {
      handler.setDefaultAction( actionFromStatus( mInvitationStatusByAtomicOperation.value( change->atomicOperationId ) ) );
    }

    switch( change->type ) {
      case IncidenceChanger::ChangeTypeCreate:
        // nothing needs to be done
      break;
      case IncidenceChanger::ChangeTypeDelete:
      {
        InvitationHandler::SendResult status;

        Incidence::Ptr incidence = change->originalItem.payload<KCalCore::Incidence::Ptr>();
        status = handler.sendIncidenceDeletedMessage( KCalCore::iTIPCancel, incidence );
        if ( change->atomicOperationId ) {
          mInvitationStatusByAtomicOperation.insert( change->atomicOperationId, status );
        }
        result = status != InvitationHandler::ResultFailAbortUpdate;
      }
      break;
      case IncidenceChanger::ChangeTypeModify:
      {
        Incidence::Ptr oldIncidence = change->originalItem.payload<KCalCore::Incidence::Ptr>();
        Incidence::Ptr newIncidence = change->originalItem.payload<KCalCore::Incidence::Ptr>();

        const bool modify = handler.handleIncidenceAboutToBeModified( newIncidence );
        if ( !modify ) {
          if ( newIncidence->type() == oldIncidence->type() ) {
            IncidenceBase *i1 = newIncidence.data();
            IncidenceBase *i2 = oldIncidence.data();
            *i1 = *i2;
          }
          result = false;
        }
      }
      break;
      default:
        Q_ASSERT( false );
        result = false;
    }
  }
  return result;
}

bool IncidenceChanger::Private::handleInvitationsAfterChange( const Change::Ptr &change )
{
  if ( mGroupwareCommunication ) {
    InvitationHandler handler( FetchJobCalendar::Ptr(), change->parentWidget ); // TODO make async
    switch( change->type ) {
      case IncidenceChanger::ChangeTypeCreate:
      {
        Incidence::Ptr incidence = change->newItem.payload<KCalCore::Incidence::Ptr>();
        const InvitationHandler::SendResult status =
          handler.sendIncidenceCreatedMessage( KCalCore::iTIPRequest, incidence );

        if ( status == InvitationHandler::ResultFailAbortUpdate ) {
          kError() << "Sending invitations failed, but did not delete the incidence";
        }

        const uint atomicOperationId = change->atomicOperationId;
        if ( atomicOperationId != 0 ) {
          mInvitationStatusByAtomicOperation.insert( atomicOperationId, status );
        }
      }
      break;
      case IncidenceChanger::ChangeTypeDelete:
      {
        Incidence::Ptr incidence = change->originalItem.payload<KCalCore::Incidence::Ptr>();
        Q_ASSERT( incidence );
        if ( !handler.thatIsMe( incidence->organizer()->email() ) ) {
          const QStringList myEmails = handler.allEmails();
          bool notifyOrganizer = false;
          for ( QStringList::ConstIterator it = myEmails.begin(); it != myEmails.end(); ++it ) {
            const QString email = *it;
            KCalCore::Attendee::Ptr me( incidence->attendeeByMail( email ) );
            if ( me ) {
              if ( me->status() == KCalCore::Attendee::Accepted ||
                   me->status() == KCalCore::Attendee::Delegated ) {
                notifyOrganizer = true;
              }
              KCalCore::Attendee::Ptr newMe( new KCalCore::Attendee( *me ) );
              newMe->setStatus( KCalCore::Attendee::Declined );
              incidence->clearAttendees();
              incidence->addAttendee( newMe );
              break;
            }
          }

          if ( notifyOrganizer ) {
            FetchJobCalendar::Ptr invalidPtr;
            MailScheduler scheduler( invalidPtr ); // TODO make async
            scheduler.performTransaction( incidence, KCalCore::iTIPReply );
          }
        }
      }
      break;
      case IncidenceChanger::ChangeTypeModify:
      {
        Incidence::Ptr oldIncidence = change->originalItem.payload<KCalCore::Incidence::Ptr>();
        Incidence::Ptr newIncidence = change->newItem.payload<KCalCore::Incidence::Ptr>();
        if ( mInvitationStatusByAtomicOperation.contains( change->atomicOperationId ) ) {
          handler.setDefaultAction( actionFromStatus( mInvitationStatusByAtomicOperation.value( change->atomicOperationId ) ) );
        }
        const bool attendeeStatusChanged = myAttendeeStatusChanged( newIncidence,
                                                                    oldIncidence,
                                                                    handler.allEmails() );
        InvitationHandler::SendResult status = handler.sendIncidenceModifiedMessage( KCalCore::iTIPRequest,
                                                                                     newIncidence,
                                                                                     attendeeStatusChanged );

        if ( change->atomicOperationId != 0 ) {
          mInvitationStatusByAtomicOperation.insert( change->atomicOperationId, status );
        }
      }
      break;
      default:
        Q_ASSERT( false );
        return false;
    }
  }
  return true;
}

/** static */
bool IncidenceChanger::Private::myAttendeeStatusChanged( const Incidence::Ptr &newInc,
                                                         const Incidence::Ptr &oldInc,
                                                         const QStringList &myEmails )
{
  Q_ASSERT( newInc );
  Q_ASSERT( oldInc );
  const Attendee::Ptr oldMe = oldInc->attendeeByMails( myEmails );
  const Attendee::Ptr newMe = newInc->attendeeByMails( myEmails );

  return oldMe && newMe && oldMe->status() != newMe->status();
}

IncidenceChanger::IncidenceChanger( QObject *parent ) : QObject( parent )
                                                      , d( new Private( /**history=*/true, this ) )
{
}

IncidenceChanger::IncidenceChanger( bool enableHistory, QObject *parent ) : QObject( parent )
                                                      , d( new Private( enableHistory, this ) )
{
}

IncidenceChanger::~IncidenceChanger()
{
  delete d;
}

int IncidenceChanger::createIncidence( const Incidence::Ptr &incidence,
                                       const Collection &collection,
                                       QWidget *parent )
{
  //kDebug();
  if ( !incidence ) {
    kWarning() << "An invalid payload is not allowed.";
    d->cancelTransaction();
    return -1;
  }

  const uint atomicOperationId = d->mBatchOperationInProgress ? d->mLatestAtomicOperationId : 0;

  const Change::Ptr change( new CreationChange( this, ++d->mLatestChangeId,
                                                atomicOperationId, parent ) );
  Collection collectionToUse;

  const int changeId = change->id;
  Q_ASSERT( !( d->mBatchOperationInProgress && !d->mAtomicOperations.contains( atomicOperationId ) ) );
  if ( d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback() ) {
    // rollback is in progress, no more changes allowed.
    // TODO: better message, and i18n
    const QString errorMessage = "One change belonging to a group of changes failed."
                                 "Undoing in progress.";
    kWarning() << errorMessage;

    change->resultCode = ResultCodeRolledback;
    change->errorString = errorMessage;
    d->cleanupTransaction();
    return changeId;
  }

  if ( collection.isValid() && d->hasRights( collection, ChangeTypeCreate ) ) {
    // The collection passed always has priority
    collectionToUse = collection;
  } else {
    switch( d->mDestinationPolicy ) {
      case DestinationPolicyDefault:
        if ( d->mDefaultCollection.isValid() &&
             d->hasRights( d->mDefaultCollection, ChangeTypeCreate ) ) {
          collectionToUse = d->mDefaultCollection;
          break;
        }
        kWarning() << "Destination policy is to use the default collection."
                   << "But it's invalid or doesn't have proper ACLs."
                   << "isValid = "  << d->mDefaultCollection.isValid()
                   << "has ACLs = " << d->hasRights( d->mDefaultCollection,
                                                     ChangeTypeCreate );
        // else fallthrough, and ask the user.
      case DestinationPolicyAsk:
      {
        int dialogCode;
        const QStringList mimeTypes( incidence->mimeType() );
        //TODO: selectCollection uses an .exec()
        collectionToUse = selectCollection( parent, dialogCode /*by-ref*/, mimeTypes,
                                            d->mDefaultCollection );
        if ( dialogCode != QDialog::Accepted ) {
          kDebug() << "User canceled collection choosing";
          change->resultCode = ResultCodeUserCanceled;
          d->cancelTransaction();
          return changeId;
        }

        if ( collectionToUse.isValid() && !d->hasRights( collectionToUse, ChangeTypeCreate ) ) {
          kWarning() << "No ACLs for incidence creation";
          const QString errorMessage = d->showErrorDialog( ResultCodePermissions, parent );
          change->resultCode = ResultCodePermissions;
          change->errorString = errorMessage;
          d->cancelTransaction();
          return changeId;
        }

        // TODO: add unit test for these two situations after reviewing API
        if ( !collectionToUse.isValid() ) {
          kError() << "Invalid collection selected. Can't create incidence.";
          change->resultCode = ResultCodeInvalidUserCollection;
          const QString errorString = d->showErrorDialog( ResultCodeInvalidUserCollection, parent );
          change->errorString = errorString;
          d->cancelTransaction();
          return changeId;
        }
      }
      break;
      case DestinationPolicyNeverAsk:
      {
        const bool hasRights = d->hasRights( d->mDefaultCollection, ChangeTypeCreate );
        if ( d->mDefaultCollection.isValid() && hasRights ) {
          collectionToUse = d->mDefaultCollection;
        } else {
          const QString errorString = d->showErrorDialog( ResultCodeInvalidDefaultCollection, parent );
          kError() << errorString << "; rights are " << hasRights;
          change->resultCode = hasRights ? ResultCodeInvalidDefaultCollection :
                                           ResultCodePermissions;
          change->errorString = errorString;
          d->cancelTransaction();
          return changeId;
        }
      }
      break;
    default:
      // Never happens
      Q_ASSERT_X( false, "createIncidence()", "unknown destination policy" );
      d->cancelTransaction();
      return -1;
    }
  }

  Item item;
  item.setPayload<Incidence::Ptr>( incidence );
  item.setMimeType( incidence->mimeType() );

  ItemCreateJob *createJob = new ItemCreateJob( item, collectionToUse, d->parentJob( change ) );
  d->mChangeForJob.insert( createJob, change );

  if ( d->mBatchOperationInProgress ) {
    AtomicOperation *atomic = d->mAtomicOperations[d->mLatestAtomicOperationId];
    Q_ASSERT( atomic );
    atomic->addChange( change );
  }

  // QueuedConnection because of possible sync exec calls.
  connect( createJob, SIGNAL(result(KJob*)),
           d, SLOT(handleCreateJobResult(KJob*)), Qt::QueuedConnection );

  d->mChangeById.insert( changeId, change );
  return change->id;
}

int IncidenceChanger::deleteIncidence( const Item &item, QWidget *parent )
{
  Item::List list;
  list.append( item );

  return deleteIncidences( list, parent );
}

int IncidenceChanger::deleteIncidences( const Item::List &items, QWidget *parent )
{
  //kDebug();
  if ( items.isEmpty() ) {
    kError() << "Delete what?";
    d->cancelTransaction();
    return -1;
  }

  foreach( const Item &item, items ) {
    if ( !item.isValid() ) {
      kError() << "Items must be valid!";
      d->cancelTransaction();
      return -1;
    }
  }

  const uint atomicOperationId = d->mBatchOperationInProgress ? d->mLatestAtomicOperationId : 0;
  const int changeId = ++d->mLatestChangeId;
  const Change::Ptr change( new DeletionChange( this, changeId, atomicOperationId, parent ) );

  foreach( const Item &item, items ) {
    if ( !d->hasRights( item.parentCollection(), ChangeTypeDelete ) ) {
      kWarning() << "Item " << item.id() << " can't be deleted due to ACL restrictions";
      const QString errorString = d->showErrorDialog( ResultCodePermissions, parent );
      change->resultCode = ResultCodePermissions;
      change->errorString = errorString;
      d->cancelTransaction();
      return changeId;
    }
  }

  if ( !d->allowAtomicOperation( atomicOperationId, change ) ) {
    const QString errorString = d->showErrorDialog( ResultCodeDuplicateId, parent );
    change->resultCode = ResultCodeDuplicateId;
    change->errorString = errorString;
    d->cancelTransaction();
    return changeId;
  }

  Item::List itemsToDelete;
  foreach( const Item &item, items ) {
    if ( d->deleteAlreadyCalled( item.id() ) ) {
      // IncidenceChanger::deleteIncidence() called twice, ignore this one.
      kDebug() << "Item " << item.id() << " already deleted or being deleted, skipping";
    } else {
      itemsToDelete.append( item );
    }
  }

  if ( d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback() ) {
    // rollback is in progress, no more changes allowed.
    // TODO: better message, and i18n
    const QString errorMessage = "One change belonging to a group of changes failed."
                                 "Undoing in progress.";
    change->resultCode = ResultCodeRolledback;
    change->errorString = errorMessage;
    d->cleanupTransaction();
    return changeId;
  }

  if ( itemsToDelete.isEmpty() ) {
    QVector<Akonadi::Item::Id> itemIdList;
    itemIdList.append( Item().id() );
    kDebug() << "Items already deleted or being deleted, skipping";
    const QString errorMessage =
                      i18n( "That calendar item was already deleted, or currently being deleted." );
    // Queued emit because return must be executed first, otherwise caller won't know this workId
    change->resultCode = ResultCodeAlreadyDeleted;
    change->errorString = errorMessage;
    d->cancelTransaction();
    return changeId;
  }

  ItemDeleteJob *deleteJob = new ItemDeleteJob( itemsToDelete, d->parentJob( change ) );
  d->mChangeForJob.insert( deleteJob, change );
  d->mChangeById.insert( changeId, change );

  if ( d->mBatchOperationInProgress ) {
    AtomicOperation *atomic = d->mAtomicOperations[atomicOperationId];
    Q_ASSERT( atomic );
    atomic->addChange( change );
  }

  foreach( const Item &item, itemsToDelete ) {
    d->mDeletedItemIds.insert( item.id() );
  }

  // QueuedConnection because of possible sync exec calls.
  connect( deleteJob, SIGNAL(result(KJob *)),
           d, SLOT(handleDeleteJobResult(KJob *)), Qt::QueuedConnection );

  return changeId;
}

int IncidenceChanger::modifyIncidence( const Item &changedItem,
                                       const KCalCore::Incidence::Ptr &originalPayload,
                                       QWidget *parent )
{
  if ( !changedItem.isValid() || !changedItem.hasPayload<Incidence::Ptr>() ) {
    kWarning() << "An invalid item or payload is not allowed.";
    d->cancelTransaction();
    return -1;
  }

  if ( !d->hasRights( changedItem.parentCollection(), ChangeTypeModify ) ) {
    kWarning() << "Item " << changedItem.id() << " can't be deleted due to ACL restrictions";
    const int changeId = ++d->mLatestChangeId;
    const QString errorString = d->showErrorDialog( ResultCodePermissions, parent );
    emitModifyFinished( this, changeId, changedItem, ResultCodePermissions, errorString );
    d->cancelTransaction();
    return changeId;
  }

  Item originalItem;
  if ( originalPayload ) {
    originalItem = Item( changedItem );
    originalItem.setPayload<KCalCore::Incidence::Ptr>( originalPayload );
  }

  const uint atomicOperationId = d->mBatchOperationInProgress ? d->mLatestAtomicOperationId : 0;
  const int changeId = ++d->mLatestChangeId;
  ModificationChange *modificationChange = new ModificationChange( this, changeId,
                                                                   atomicOperationId, parent );
  Change::Ptr change( modificationChange );
  modificationChange->originalItem = originalItem;
  modificationChange->newItem = changedItem;
  d->mChangeById.insert( changeId, change );

  if ( !d->allowAtomicOperation( atomicOperationId, change ) ) {
    const QString errorString = d->showErrorDialog( ResultCodeDuplicateId, parent );
    change->resultCode = ResultCodeDuplicateId;
    change->errorString = errorString;
    d->cancelTransaction();
    return changeId;
  }

  if ( d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback() ) {
    // rollback is in progress, no more changes allowed.
    // TODO: better message, and i18n
    const QString errorMessage = "One change belonging to a group of changes failed."
                                 "Undoing in progress.";
    d->cleanupTransaction();
    emitModifyFinished( this, changeId, changedItem, ResultCodeRolledback, errorMessage );
  } else {
    d->performModification( change );
  }

  return changeId;
}

void IncidenceChanger::Private::performModification( Change::Ptr change )
{
  const Item::Id id = change->newItem.id();
  Akonadi::Item &newItem = change->newItem;
  Q_ASSERT( newItem.isValid() );
  Q_ASSERT( newItem.hasPayload<Incidence::Ptr>() );

  const int changeId = change->id;

  if ( deleteAlreadyCalled( id ) ) {
    // IncidenceChanger::deleteIncidence() called twice, ignore this one.
    kDebug() << "Item " << id << " already deleted or being deleted, skipping";

    // Queued emit because return must be executed first, otherwise caller won't know this workId
    emitModifyFinished( q, change->id, newItem, ResultCodeAlreadyDeleted,
                    i18n( "That calendar item was already deleted, or currently being deleted." ) );
    return;
  }

  const uint atomicOperationId = change->atomicOperationId;
  const bool hasAtomicOperationId = atomicOperationId != 0;
  if ( hasAtomicOperationId &&
       mAtomicOperations[atomicOperationId]->rolledback() ) {
    // rollback is in progress, no more changes allowed.
    // TODO: better message, and i18n
    const QString errorMessage = "One change belonging to a group of changes failed."
                                 "Undoing in progress.";

    emitModifyFinished( q, changeId, newItem, ResultCodeRolledback, errorMessage );
    return;
  }

  QHash<Akonadi::Item::Id, int> &latestRevisionByItemId =
                                                 ConflictPreventer::self()->mLatestRevisionByItemId;
  if ( latestRevisionByItemId.contains( id ) &&
       latestRevisionByItemId[id] > newItem.revision() ) {
    /* When a ItemModifyJob ends, the application can still modify the old items if the user
     * is quick because the ETM wasn't updated yet, and we'll get a STORE error, because
     * we are not modifying the latest revision.
     *
     * When a job ends, we keep the new revision in mLatestRevisionByItemId
     * so we can update the item's revision
     */
    newItem.setRevision( latestRevisionByItemId[id] );
  }

  Incidence::Ptr incidence = newItem.payload<Incidence::Ptr>();
  { // increment revision ( KCalCore revision, not akonadi )
    const int revision = incidence->revision();
    incidence->setRevision( revision + 1 );
  }

  // Dav Fix
  // Don't write back remote revision since we can't make sure it is the current one
  newItem.setRemoteRevision( QString() );

  if ( mModificationsInProgress.contains( newItem.id() ) ) {
    // There's already a ItemModifyJob running for this item ID
    // Let's wait for it to end.
    queueModification( change );
  } else {
    ItemModifyJob *modifyJob = new ItemModifyJob( newItem, parentJob( change ) );
    mChangeForJob.insert( modifyJob, change );
    mDirtyFieldsByJob.insert( modifyJob, incidence->dirtyFields() );

    if ( hasAtomicOperationId ) {
      AtomicOperation *atomic = mAtomicOperations[atomicOperationId];
      Q_ASSERT( atomic );
      atomic->addChange( change );
    }

    mModificationsInProgress[newItem.id()] = change;
    // QueuedConnection because of possible sync exec calls.
    connect( modifyJob, SIGNAL(result(KJob *)),
             SLOT(handleModifyJobResult(KJob *)), Qt::QueuedConnection );
  }
}

void IncidenceChanger::startAtomicOperation( const QString &operationDescription )
{
  //kDebug();
  if ( d->mBatchOperationInProgress ) {
    kDebug() << "An atomic operation is already in progress.";
    return;
  }

  ++d->mLatestAtomicOperationId;
  d->mBatchOperationInProgress = true;

  AtomicOperation *atomicOperation = new AtomicOperation( d->mLatestAtomicOperationId );
  atomicOperation->description = operationDescription;
  d->mAtomicOperations.insert( d->mLatestAtomicOperationId, atomicOperation );
  d->mAtomicOperationByTransaction.insert( atomicOperation->transaction, d->mLatestAtomicOperationId );

  // TODO: rename transaction
  d->connect( atomicOperation->transaction, SIGNAL(result(KJob*)),
              d, SLOT(handleTransactionJobResult(KJob*)) );
}

void IncidenceChanger::endAtomicOperation()
{
  //kDebug();
  if ( !d->mBatchOperationInProgress ) {
    kDebug() << "No atomic operation is in progress.";
    return;
  }

  Q_ASSERT_X( d->mLatestAtomicOperationId != 0,
              "IncidenceChanger::endAtomicOperation()",
              "Call startAtomicOperation() first." );

  Q_ASSERT( d->mAtomicOperations.contains(d->mLatestAtomicOperationId) );
  AtomicOperation *atomicOperation = d->mAtomicOperations[d->mLatestAtomicOperationId];
  Q_ASSERT( atomicOperation );
  atomicOperation->endCalled = true;

  const bool allJobsCompleted = !atomicOperation->pendingJobs();

  if ( allJobsCompleted && atomicOperation->rolledback() &&
       atomicOperation->transactionCompleted ) {
    // The transaction job already completed, we can cleanup:
    delete d->mAtomicOperations.take( d->mLatestAtomicOperationId );
    d->mBatchOperationInProgress = false;
  }/* else if ( allJobsCompleted ) {
    Q_ASSERT( atomicOperation->transaction );
    atomicOperation->transaction->commit(); we using autocommit now
  }*/
}

void IncidenceChanger::setShowDialogsOnError( bool enable )
{
  d->mShowDialogsOnError = enable;
}

bool IncidenceChanger::showDialogsOnError() const
{
  return d->mShowDialogsOnError;
}

void IncidenceChanger::setRespectsCollectionRights( bool respects )
{
  d->mRespectsCollectionRights = respects;
}

bool IncidenceChanger::respectsCollectionRights() const
{
  return d->mRespectsCollectionRights;
}

void IncidenceChanger::setDestinationPolicy( IncidenceChanger::DestinationPolicy destinationPolicy )
{
  d->mDestinationPolicy = destinationPolicy;
}

IncidenceChanger::DestinationPolicy IncidenceChanger::destinationPolicy() const
{
  return d->mDestinationPolicy;
}

void IncidenceChanger::setDefaultCollection( const Akonadi::Collection &collection )
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

void IncidenceChanger::setHistoryEnabled( bool enable )
{
  if ( d->mUseHistory != enable ) {
    d->mUseHistory = enable;
    if ( enable && !d->mHistory )
      d->mHistory = new History( this );
  }
}

History* IncidenceChanger::history() const
{
  return d->mHistory;
}

void IncidenceChanger::setGroupwareCommuniation( bool enabled )
{
  d->mGroupwareCommunication = enabled;
}

bool IncidenceChanger::groupwareCommunication() const
{
  return d->mGroupwareCommunication;
}

QString IncidenceChanger::Private::showErrorDialog( IncidenceChanger::ResultCode resultCode,
                                                    QWidget *parent )
{
  QString errorString;
  switch( resultCode ) {
    case IncidenceChanger::ResultCodePermissions:
      errorString = i18n( "Operation can not be performed due to ACL restrictions" );
      break;
    case IncidenceChanger::ResultCodeInvalidUserCollection:
      errorString = i18n( "The chosen collection is invalid" );
      break;
    case IncidenceChanger::ResultCodeInvalidDefaultCollection:
      errorString = i18n( "Default collection is invalid or doesn't have proper ACLs"
                          " and DestinationPolicyNeverAsk was used" );
      break;
    default:
    case IncidenceChanger::ResultCodeDuplicateId:
      errorString = i18n( "Duplicate item id in a group operation");
      break;
    Q_ASSERT( false );
    return QString( i18n( "Unknown error" ) );
  }

  if ( mShowDialogsOnError ) {
    KMessageBox::sorry( parent, errorString );
  }

  return errorString;
}

void IncidenceChanger::Private::cancelTransaction()
{
  if ( mBatchOperationInProgress ) {
    mAtomicOperations[mLatestAtomicOperationId]->setRolledback();
  }
}

void IncidenceChanger::Private::cleanupTransaction()
{
  Q_ASSERT( mAtomicOperations.contains(mLatestAtomicOperationId) );
  AtomicOperation *operation = mAtomicOperations[mLatestAtomicOperationId];
  Q_ASSERT( operation );
  Q_ASSERT( operation->rolledback() );
  if ( !operation->pendingJobs() && operation->endCalled && operation->transactionCompleted ) {
    delete mAtomicOperations.take(mLatestAtomicOperationId);
    mBatchOperationInProgress = false;
  }
}

bool IncidenceChanger::Private::allowAtomicOperation( int atomicOperationId,
                                                      const Change::Ptr &change ) const
{
  bool allow = true;
  if ( atomicOperationId > 0 ) {
    Q_ASSERT( mAtomicOperations.contains( atomicOperationId ) );
    AtomicOperation *operation = mAtomicOperations.value( atomicOperationId );

    if ( change->type == ChangeTypeCreate ) {
      allow = true;
    } else if ( change->type == ChangeTypeModify ) {
      allow = !operation->mItemIdsInOperation.contains( change->newItem.id() );
    } else if ( change->type == ChangeTypeDelete ) {
      DeletionChange::Ptr deletion = change.staticCast<DeletionChange>();
      foreach( Akonadi::Item::Id id, deletion->mItemIds ) {
        if ( operation->mItemIdsInOperation.contains( id ) ) {
          allow = false;
          break;
        }
      }
    }
  }

  if ( !allow ) {
    kWarning() << "Each change belonging to a group operation"
               << "must have a different Akonadi::Item::Id";
  }

  return allow;
}

/**reimp*/
void ModificationChange::emitCompletionSignal()
{
  emitModifyFinished( changer, id, newItem, resultCode, errorString );
}

/**reimp*/
void CreationChange::emitCompletionSignal()
{
  // Does a queued emit, with QMetaObject::invokeMethod
  emitCreateFinished( changer, id, newItem, resultCode, errorString );
}

/**reimp*/
void DeletionChange::emitCompletionSignal()
{
  emitDeleteFinished( changer, id, mItemIds, resultCode, errorString );
}
