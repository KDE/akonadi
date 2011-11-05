/*
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (C) 2010-2011 Sérgio Martins <iamsergio@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#include "incidencechanger.h"
#include "incidencechanger_p.h"

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

}

// Does a queued emit, with QMetaObject::invokeMethod
static void Akonadi::emitCreateFinished( IncidenceChanger *changer,
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
static void Akonadi::emitModifyFinished( IncidenceChanger *changer,
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
static void Akonadi::emitDeleteFinished( IncidenceChanger *changer,
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

IncidenceChanger::Private::Private( IncidenceChanger *qq ) : q( qq )
{
  mLatestChangeId = 0;
  mShowDialogsOnError = true;
  mDestinationPolicy = DestinationPolicyDefault;
  mRespectsCollectionRights = false;
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

void IncidenceChanger::Private::queueModification( Change::Ptr change )
{
  // If there's already a change queued we just discard it
  // and send the newer change, which already includes
  // previous modifications
  const Akonadi::Item::Id id = change->newItem.id();
  if ( mQueuedModifications.contains( id ) ) {
    mQueuedModifications.take( id );
  }

  mQueuedModifications[id] = change;
}

void IncidenceChanger::Private::performNextModification( Akonadi::Item::Id id )
{
  mModificationsInProgress.remove( id );

  if ( mQueuedModifications.contains( id ) ) {
    Change::Ptr change = mQueuedModifications[id];
    performModification( change );
  }
}

void IncidenceChanger::Private::handleTransactionJobResult( KJob *job )
{
  TransactionSequence *transaction = qobject_cast<TransactionSequence*>( job );
  Q_ASSERT( transaction );
  Q_ASSERT( mAtomicOperationByTransaction.contains( transaction ) );

  const uint atomicOperationId = mAtomicOperationByTransaction.take( transaction );

  AtomicOperation *operation = mAtomicOperations[atomicOperationId];
  Q_ASSERT( operation->id == atomicOperationId );

  if ( job->error() ) {
    operation->rolledback = true;
    kError() << "Transaction failed, everything was rolledback. "
             << job->errorString();
  } else {
    Q_ASSERT( operation->endCalled );
    Q_ASSERT( !operation->pendingJobs() );
  }

  if ( !operation->pendingJobs() && operation->endCalled ) {
    delete mAtomicOperations.take( atomicOperationId );
    mBatchOperationInProgress = false;
  }
  //TODO: será possivel que umas chegam com sucesso, nos emitimos, e depois ha uma que rollbaca?
  // Só deviamos emitir tudo qdo a transacao chegar.
}

void IncidenceChanger::Private::handleCreateJobResult( KJob *job )
{
  QString errorString;
  ResultCode resultCode = ResultCodeSuccess;

  Change::Ptr change = mChangeForJob.take( job );
  mChangeById.remove( change->id );

  const ItemCreateJob *j = qobject_cast<const ItemCreateJob*>( job );
  Q_ASSERT( j );
  Akonadi::Item item = j->item();

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

    QString description;
    if ( change->atomicOperationId != 0 ) {
      AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
      a->numCompletedChanges++;
      description = a->description;
    }

    change->newItem     = item;
    change->errorString = errorString;
    change->resultCode  = resultCode;
    // puff, change finally goes out of scope, and emits the incidenceCreated signal.
  }
}

void IncidenceChanger::Private::handleDeleteJobResult( KJob *job )
{
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
      mLatestRevisionByItemId.remove( item.id() );

      QString description;
      if ( change->atomicOperationId != 0 ) {
        AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
        a->numCompletedChanges++;
        description = a->description;
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
      KMessageBox::sorry( change->parentWidget, i18n( "Error while trying to modify calendar item. Error was: %1",
                                                 errorString ) );
    }
  } else { // success

    mLatestRevisionByItemId[item.id()] = item.revision();

    QString description;
    if ( change->atomicOperationId != 0 ) {
      AtomicOperation *a = mAtomicOperations[change->atomicOperationId];
      a->numCompletedChanges++;
      description = a->description;
    }
    change->newItem = item;
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

IncidenceChanger::IncidenceChanger( QObject *parent ) : QObject( parent )
                                                      , d( new Private( this ) )
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
  if ( !incidence ) {
    kWarning() << "An invalid payload is not allowed.";
    return -1;
  }

  const uint atomicOperationId = d->mBatchOperationInProgress ? d->mLatestAtomicOperationId : 0;

  const Change::Ptr change( new CreationChange( this, ++d->mLatestChangeId,
                                                atomicOperationId, parent ) );
  Collection collectionToUse;

  const int changeId = change->id;
  Q_ASSERT( !( d->mBatchOperationInProgress && !d->mAtomicOperations.contains( atomicOperationId ) ) );
  if ( d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback ) {
    // rollback is in progress, no more changes allowed.
    // TODO: better message, and i18n
    const QString errorMessage = "One change belonging to a group of changes failed."
                                 "Undoing in progress.";

    change->resultCode = ResultCodeRolledback;
    change->errorString = errorMessage;
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
          return changeId;
        }

        if ( collectionToUse.isValid() && !d->hasRights( collectionToUse, ChangeTypeCreate ) ) {
          kWarning() << "No ACLs for incidence creation";
          const QString errorMessage = d->showErrorDialog( ResultCodePermissions, parent );
          change->resultCode = ResultCodePermissions;
          change->errorString = errorMessage;
          return changeId;
        }

        // TODO: add unit test for these two situations after reviewing API
        if ( !collectionToUse.isValid() ) {
          kError() << "Invalid collection selected. Can't create incidence.";
          change->resultCode = ResultCodeInvalidUserCollection;
          const QString errorString = d->showErrorDialog( ResultCodeInvalidUserCollection, parent );
          change->errorString = errorString;
          return changeId;
        }
      }
      break;
      case DestinationPolicyNeverAsk:
      {
        const bool rights = d->hasRights( d->mDefaultCollection, ChangeTypeCreate );
        if ( d->mDefaultCollection.isValid() && rights ) {
          collectionToUse = d->mDefaultCollection;
        } else {
          const QString errorString = d->showErrorDialog( ResultCodeInvalidDefaultCollection, parent );
          kError() << errorString << "; rights are " << rights;
          change->resultCode = ResultCodeInvalidDefaultCollection;
          change->errorString = errorString;
          return changeId;
        }
      }
      break;
    default:
      // Never happens
      Q_ASSERT_X( false, "createIncidence()", "unknown destination policy" );
      return -1;
    }
  }

  Item item;
  item.setPayload<Incidence::Ptr>( incidence );
  item.setMimeType( incidence->mimeType() );

  ItemCreateJob *createJob = new ItemCreateJob( item, collectionToUse );
  d->mChangeForJob.insert( createJob, change );

  if ( d->mBatchOperationInProgress ) {
    AtomicOperation *atomic = d->mAtomicOperations[d->mLatestAtomicOperationId];
    ++atomic->numChanges;
    createJob->setParent( atomic->transaction );
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
  if ( items.isEmpty() ) {
    kError() << "Delete what?";
    return -1;
  }

  foreach( const Item &item, items ) {
    if ( !item.isValid() ) {
      kError() << "Items must be valid!";
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
      return changeId;
    }    
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

  if ( d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback ) {
    // rollback is in progress, no more changes allowed.
    // TODO: better message, and i18n
    const QString errorMessage = "One change belonging to a group of changes failed."
                                 "Undoing in progress.";
    change->resultCode = ResultCodeRolledback;
    change->errorString = errorMessage;
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
    return changeId;
  }

  ItemDeleteJob *deleteJob = new ItemDeleteJob( itemsToDelete );
  d->mChangeForJob.insert( deleteJob, change );
  d->mChangeById.insert( changeId, change );

  if ( d->mBatchOperationInProgress ) {
    AtomicOperation *atomic = d->mAtomicOperations[atomicOperationId];
    ++atomic->numChanges;
    deleteJob->setParent( atomic->transaction );
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
    return -1;
  }

  if ( !d->hasRights( changedItem.parentCollection(), ChangeTypeModify ) ) {
    kWarning() << "Item " << changedItem.id() << " can't be deleted due to ACL restrictions";
    const int changeId = ++d->mLatestChangeId;
    const QString errorString = d->showErrorDialog( ResultCodePermissions, parent );
    emitModifyFinished( this, changeId, changedItem, ResultCodePermissions, errorString );
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

  if ( d->mBatchOperationInProgress && d->mAtomicOperations[atomicOperationId]->rolledback ) {
    // rollback is in progress, no more changes allowed.
    // TODO: better message, and i18n
    const QString errorMessage = "One change belonging to a group of changes failed."
                                 "Undoing in progress.";

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
       mAtomicOperations[atomicOperationId]->rolledback ) {
    // rollback is in progress, no more changes allowed.
    // TODO: better message, and i18n
    const QString errorMessage = "One change belonging to a group of changes failed."
                                 "Undoing in progress.";

    emitModifyFinished( q, changeId, newItem, ResultCodeRolledback, errorMessage );
    return;
  }

  if ( mLatestRevisionByItemId.contains( id ) &&
       mLatestRevisionByItemId[id] > newItem.revision() ) {
    /* When a ItemModifyJob ends, the application can still modify the old items if the user
     * is quick because the ETM wasn't updated yet, and we'll get a STORE error, because
     * we are not modifying the latest revision.
     *
     * When a job ends, we keep the new revision in m_latestVersionByItemId
     * so we can update the item's revision
     */
    newItem.setRevision( mLatestRevisionByItemId[id] );
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
    ItemModifyJob *modifyJob = new ItemModifyJob( newItem );
    mChangeForJob.insert( modifyJob, change );
    mDirtyFieldsByJob.insert( modifyJob, incidence->dirtyFields() );

    if ( hasAtomicOperationId ) {
      AtomicOperation *atomic = mAtomicOperations[atomicOperationId];
      atomic->numChanges++;
      modifyJob->setParent( atomic->transaction );
    }

    mModificationsInProgress[newItem.id()] = change;
    // QueuedConnection because of possible sync exec calls.
    connect( modifyJob, SIGNAL(result(KJob *)),
             SLOT(handleModifyJobResult(KJob *)), Qt::QueuedConnection );
  }
}

void IncidenceChanger::startAtomicOperation( const QString &operationDescription )
{
  ++d->mLatestAtomicOperationId;
  Q_ASSERT_X( !d->mBatchOperationInProgress, "IncidenceChanger::startAtomicOperation()",
              "Call endAtomicOperation() first." );
  d->mBatchOperationInProgress = true;

  AtomicOperation *atomicOperation = new AtomicOperation( d->mLatestAtomicOperationId );
  atomicOperation->description = operationDescription;

  d->mAtomicOperations.insert( d->mLatestAtomicOperationId, atomicOperation );
  d->mAtomicOperationByTransaction.insert( atomicOperation->transaction, d->mLatestAtomicOperationId );

  // TODO: rename transaction
  d->connect( atomicOperation->transaction, SIGNAL(result(KJob*)),
              d, SLOT(handleTransactionJobResult(KJob*)), Qt::QueuedConnection );
}

void IncidenceChanger::endAtomicOperation()
{
  Q_ASSERT_X( d->mLatestAtomicOperationId != 0,
              "IncidenceChanger::endAtomicOperation()",
              "Call startAtomicOperation() first." );

  Q_ASSERT_X( d->mBatchOperationInProgress, "IncidenceChanger::endAtomicOperation()",
              "Call startAtomicOperation() first." );

  AtomicOperation *atomicOperation = d->mAtomicOperations[d->mLatestAtomicOperationId];
  atomicOperation->endCalled = true;

  const bool allJobsCompleted = !atomicOperation->pendingJobs();

  if ( allJobsCompleted && atomicOperation->rolledback ) {
    // The transaction job already completed, we can cleanup:
    delete d->mAtomicOperations.take( d->mLatestAtomicOperationId );
    d->mBatchOperationInProgress = false;
  } else if ( allJobsCompleted ) {
    Q_ASSERT( atomicOperation->transaction );
    atomicOperation->transaction->commit();
  }
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

QString IncidenceChanger::Private::showErrorDialog( IncidenceChanger::ResultCode resultCode, QWidget *parent )
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
                          "and DestinationPolicyNeverAsk was used" );
      break;
    default:
      Q_ASSERT( false );
      return QString( i18n( "Unknown error" ) );
  }

  if ( mShowDialogsOnError ) {
    KMessageBox::sorry( parent, errorString );
  }

  return errorString;
}

  