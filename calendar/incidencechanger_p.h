/*
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Author: Sergio Martins, <sergio.martins@kdab.com>

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
#ifndef AKONADI_INCIDENCECHANGER_P_H
#define AKONADI_INCIDENCECHANGER_P_H

#include "incidencechanger.h"
#include "itiphandlerhelper_p.h"
#include "history.h"

#include <akonadi/item.h>
#include <akonadi/collection.h>
#include <akonadi/transactionsequence.h>

#include <QSet>
#include <QObject>
#include <QPointer>
#include <QVector>

class KJob;
class QWidget;

namespace Akonadi {

class TransactionSequence;

class Change {

public:
  typedef QSharedPointer<Change> Ptr;
  Change( IncidenceChanger *incidenceChanger, int changeId,
          IncidenceChanger::ChangeType changeType, uint operationId,
          QWidget *parent ) : id( changeId )
                            , type( changeType )
                            , recordToHistory( incidenceChanger->historyEnabled() )
                            , parentWidget( parent )
                            , atomicOperationId( operationId )
                            , resultCode( Akonadi::IncidenceChanger::ResultCodeSuccess )
                            , completed( false )
                            , queuedModification( false )
                            , useGroupwareCommunication( incidenceChanger->groupwareCommunication() )
                            , changer( incidenceChanger )
  {
  }

  virtual ~Change()
  {
    if ( parentChange ) {
      parentChange->childAboutToDie( this );
    }
  }

  virtual void childAboutToDie( Change *child )
  {
    Q_UNUSED( child );
  }

  virtual void emitCompletionSignal() = 0;

  const int id;
  const IncidenceChanger::ChangeType type;
  const bool recordToHistory;
  const QPointer<QWidget> parentWidget;
  uint atomicOperationId;

  // If this change is internal, i.e. not initiated by the user, mParentChange will
  // contain the non-internal change.
  QSharedPointer<Change> parentChange;

  Akonadi::Item::List originalItems;
  Akonadi::Item newItem;

  QString errorString;
  IncidenceChanger::ResultCode resultCode;
  bool completed;
  bool queuedModification;
  bool useGroupwareCommunication;
protected:
  IncidenceChanger *const changer;
};

class ModificationChange : public Change
{
public:
  typedef QSharedPointer<ModificationChange> Ptr;
  ModificationChange( IncidenceChanger *changer, int id, uint atomicOperationId,
                      QWidget *parent ) : Change( changer, id,
                                                  IncidenceChanger::ChangeTypeModify,
                                                  atomicOperationId, parent )
  {
  }

  ~ModificationChange()
  {
    if ( !parentChange )
      emitCompletionSignal();
  }

  /**reimp*/
  void emitCompletionSignal();
};

class CreationChange : public Change
{
public:
  typedef QSharedPointer<CreationChange> Ptr;
  CreationChange( IncidenceChanger *changer, int id, uint atomicOperationId,
                  QWidget *parent ) : Change( changer, id, IncidenceChanger::ChangeTypeCreate,
                                              atomicOperationId, parent )
  {
  }

  ~CreationChange()
  {
    //kDebug() << "CreationChange::~ will emit signal with " << resultCode;
    if ( !parentChange )
      emitCompletionSignal();
  }

  /**reimp*/
  void emitCompletionSignal();

  Akonadi::Collection mUsedCol1lection;
};

class DeletionChange : public Change
{
public:
  typedef QSharedPointer<DeletionChange> Ptr;
  DeletionChange( IncidenceChanger *changer, int id, uint atomicOperationId,
                  QWidget *parent ) : Change( changer, id, IncidenceChanger::ChangeTypeDelete,
                                              atomicOperationId, parent )
  {
  }

  ~DeletionChange()
  {
    //kDebug() << "DeletionChange::~ will emit signal with " << resultCode;
    if ( !parentChange )
      emitCompletionSignal();
  }

  /**reimp*/
  void emitCompletionSignal();

  QVector<Akonadi::Item::Id> mItemIds;
};

struct AtomicOperation {
  uint id;

  // To make sure they are not repeated
  QSet<Akonadi::Item::Id> mItemIdsInOperation;

  // After endAtomicOperation() is called we don't accept more changes
  bool endCalled;

  // Number of completed changes(jobs)
  int numCompletedChanges;
  QString description;
  bool transactionCompleted;

  AtomicOperation( IncidenceChanger::Private *icp, uint ident );

  ~AtomicOperation()
  {
    //kDebug() << "AtomicOperation::~ " << wasRolledback << changes.count();
    if ( wasRolledback ) {
      for ( int i=0; i<changes.count(); ++i ) {
        // When a job that can finish successfully is aborted because the transaction failed
        // because of some other job, akonadi is returning an Unknown error
        // which isnt very specific
        if ( changes[i]->completed &&
              ( changes[i]->resultCode == IncidenceChanger::ResultCodeSuccess ||
                ( changes[i]->resultCode == IncidenceChanger::ResultCodeJobError &&
                  changes[i]->errorString == QLatin1String( "Unknown error." ) ) ) ) {
          changes[i]->resultCode = IncidenceChanger::ResultCodeRolledback;
        }
      }
    }
  }

  // Did all jobs return ?
  bool pendingJobs() const
  {
    return changes.count() > numCompletedChanges;
  }

  void setRolledback()
  {
    //kDebug() << "AtomicOperation::setRolledBack()";
    wasRolledback = true;
    transaction()->rollback();
  }

  bool rolledback() const
  {
    return wasRolledback;
  }

  void addChange( const Change::Ptr &change )
  {
    if ( change->type == IncidenceChanger::ChangeTypeDelete ) {
      DeletionChange::Ptr deletion = change.staticCast<DeletionChange>();
      foreach( Akonadi::Item::Id id, deletion->mItemIds ) {
        Q_ASSERT( !mItemIdsInOperation.contains( id ) );
        mItemIdsInOperation.insert( id );
      }
    } else if ( change->type == IncidenceChanger::ChangeTypeModify ) {
      Q_ASSERT( !mItemIdsInOperation.contains( change->newItem.id() ) );
      mItemIdsInOperation.insert( change->newItem.id() );
    }

    changes << change;
  }

  Akonadi::TransactionSequence *transaction();

private:
  QVector<Change::Ptr> changes;
  bool wasRolledback;
  Akonadi::TransactionSequence *m_transaction; // constructed in first use
  IncidenceChanger::Private *m_incidenceChangerPrivate;
};

class IncidenceChanger::Private : public QObject
{
  Q_OBJECT
public:
  explicit Private( bool enableHistory, IncidenceChanger *mIncidenceChanger );
  ~Private();

  /**
      Returns true if, for a specific item, an ItemDeleteJob is already running,
      or if one already run successfully.
  */
  bool deleteAlreadyCalled( Akonadi::Item::Id id ) const;

  QString showErrorDialog( Akonadi::IncidenceChanger::ResultCode, QWidget *parent );

  void setChangeInternal( int changeId );

  void adjustRecurrence( const KCalCore::Incidence::Ptr &originalIncidence,
                         const KCalCore::Incidence::Ptr &incidence );

  bool hasRights( const Akonadi::Collection &collection, IncidenceChanger::ChangeType ) const;
  void queueModification( Change::Ptr );
  void performModification( Change::Ptr );
  bool atomicOperationIsValid( uint atomicOperationId ) const;
  Akonadi::Job* parentJob( const Change::Ptr & ) const;
  void cancelTransaction();
  void cleanupTransaction();
  bool allowAtomicOperation( int atomicOperationId, const Change::Ptr &change ) const;

  bool handleInvitationsBeforeChange( const Change::Ptr &change );
  bool handleInvitationsAfterChange( const Change::Ptr &change );
  static bool myAttendeeStatusChanged( const KCalCore::Incidence::Ptr &newIncidence,
                                        const KCalCore::Incidence::Ptr &oldIncidence,
                                        const QStringList &myEmails );

public Q_SLOTS:
  void handleCreateJobResult( KJob* );
  void handleModifyJobResult( KJob* );
  void handleDeleteJobResult( KJob* );
  void handleTransactionJobResult( KJob* );
  void performNextModification( Akonadi::Item::Id id );

public:
  int mLatestChangeId;
  QHash<const KJob*,Change::Ptr> mChangeForJob;
  bool mShowDialogsOnError;
  Akonadi::Collection mDefaultCollection;
  DestinationPolicy mDestinationPolicy;
  QVector<Akonadi::Item::Id> mDeletedItemIds;

  History *mHistory;
  bool mUseHistory;

  /**
    Queue modifications by ID. We can only send a modification to akonadi when the previous
    one ended.

    The container doesn't look like a queue because of an optimization: if there's a modification
    A in progress, a modification B waiting (queued), and then a new one C comes in,
    we just discard B, and queue C. The queue always has 1 element max.
  */
  QHash<Akonadi::Item::Id,Change::Ptr> mQueuedModifications;

  /**
      So we know if there's already a modification in progress
    */
  QHash<Akonadi::Item::Id,Change::Ptr> mModificationsInProgress;

  QHash<int,Change::Ptr> mChangeById;

  /**
      Indexed by atomic operation id.
  */
  QHash<uint,AtomicOperation*> mAtomicOperations;

  bool mRespectsCollectionRights;
  bool mGroupwareCommunication;

  QHash<Akonadi::TransactionSequence*, uint> mAtomicOperationByTransaction;
  QHash<uint,ITIPHandlerHelper::SendResult> mInvitationStatusByAtomicOperation;

  uint mLatestAtomicOperationId;
  bool mBatchOperationInProgress;
  Akonadi::Collection mLastCollectionUsed;
  bool mAutoAdjustRecurrence;

  QMap<KJob *, QSet<KCalCore::IncidenceBase::Field> > mDirtyFieldsByJob;

private:
  IncidenceChanger *q;
};

}

#endif
