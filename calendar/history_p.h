/*
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

#ifndef AKONADI_HISTORY_P_H
#define AKONADI_HISTORY_P_H

#include "history.h"
#include "incidencechanger.h"
#include <KCalCore/Incidence>
#include <KCalUtils/Stringify>
#include <Akonadi/Collection>
#include <KLocale>
#include <QPointer>
#include <QStack>

using namespace Akonadi;
using namespace KCalCore;

namespace Akonadi {

  class History;

  enum OperationType {
    TypeNone,
    TypeUndo,
    TypeRedo
  };

  class Entry : public QObject {
    Q_OBJECT
    public:
      typedef QSharedPointer<Entry> Ptr;
      Entry( const Akonadi::Item &item, const QString &description, History *qq );
      virtual void updateIds( Item::Id oldId, Item::Id newId );
      void doIt( OperationType );

      Item::Id mNewId; // The change might result in a new id.
      QString mDescription;
      Akonadi::Item mItem;
    Q_SIGNALS:
      void finished( Akonadi::IncidenceChanger::ResultCode, const QString &errorString );
    protected:
      virtual bool undo() = 0;
      virtual bool redo() = 0;
      QWidget* currentParent() const;
      int mWaitingForChangeId;
      IncidenceChanger *mChanger;
      QHash<Akonadi::Item::Id,int> mLatestRevisionByItemId;
      History *q;
    private:
      Q_DISABLE_COPY(Entry);
  };

  class History::Private : public QObject {
    Q_OBJECT
    public:
      Private( History *qq );
      ~Private(){}
      void doIt( OperationType );
      void stackEntry( const Entry::Ptr &entry );
      void updateIds( Item::Id oldId, Item::Id newId );
      void finishOperation( int changeId, History::ResultCode, const QString &errorString );
      QStack<Entry::Ptr>& destinationStack();
      QStack<Entry::Ptr>& stack( OperationType );
      QStack<Entry::Ptr>& stack();
      void undoOrRedo( OperationType, QWidget *parent );

      void emitDone( OperationType, History::ResultCode );

      bool isUndoAvailable() const;
      bool isRedoAvailable() const;

      IncidenceChanger *mChanger;

      QStack<Entry::Ptr> mUndoStack;
      QStack<Entry::Ptr> mRedoStack;

      OperationType mOperationTypeInProgress;

      Entry::Ptr mEntryInProgress;

      QString mLastErrorString;
      bool mUndoAllInProgress;

      /**
       * When recordCreation/Deletion/Modification is called and an undo operation is already in progress
       * the entry is added here.
       */
      QVector<Entry::Ptr> mQueuedEntries;
      bool mEnabled;
      QPointer<QWidget> mCurrentParent;

    public Q_SLOTS:
      void handleFinished( Akonadi::IncidenceChanger::ResultCode, const QString &errorString );

    private:
      History *q;
  };

  Entry::Entry( const Akonadi::Item &item, const QString &description,
                History *qq ) : QObject()
                              , mNewId( -1 )
                              , mDescription( description )
                              , mItem( item )
                              , mWaitingForChangeId( -1 )
                              , mChanger( qq->d->mChanger )
                              , q( qq )
  {
    QMetaObject::connectSlotsByName( this );
  }

  QWidget* Entry::currentParent() const
  {
    return q->d->mCurrentParent;
  }

  void Entry::updateIds( Item::Id oldId, Item::Id newId )
  {
    Q_ASSERT( newId != -1 );
    if ( mItem.id() == oldId )
      mItem.setId( newId );
  }

  void Entry::doIt( OperationType type )
  {
    // We don't want incidence changer to re-record this stuff
    const bool oldHistoryEnabled = mChanger->historyEnabled();
    mChanger->setHistoryEnabled( false );
    mNewId = -1;

    bool result = false;
    if ( type == TypeRedo )
      result = redo();
    else if ( type == TypeUndo )
      result = undo();
    else
      Q_ASSERT( false );

    if ( !result )
      emit finished( IncidenceChanger::ResultCodeJobError, i18n( "General error" ) );

    mChanger->setHistoryEnabled( oldHistoryEnabled );
  }

  class CreationEntry : public Entry {
    Q_OBJECT
    public:
      typedef QSharedPointer<CreationEntry> Ptr;
      CreationEntry( const Akonadi::Item &item, const QString &description,
                     History *q ) : Entry( item, description, q )
      {
        QMetaObject::connectSlotsByName( this );
        mLatestRevisionByItemId.insert( item.id(), item.revision() );
        const Incidence::Ptr incidence = mItem.payload<KCalCore::Incidence::Ptr>();
        mDescription =  i18n( "%1 creation",
                              KCalUtils::Stringify::incidenceType( incidence->type() ) );
        connect( mChanger, SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
                 SLOT(onCreateFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );
        connect( mChanger, SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
                 SLOT(onDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)) );
      }

      /**reimp*/
      bool undo()
      {
        mWaitingForChangeId = mChanger->deleteIncidence( mItem, currentParent() );
        return mWaitingForChangeId != -1;
      }

      bool redo()
      {
        // TODO: pass mCollection
        Q_ASSERT( mItem.hasPayload<KCalCore::Incidence::Ptr>() );
        mWaitingForChangeId = mChanger->createIncidence( mItem.payload<KCalCore::Incidence::Ptr>(),
                                                         Collection(),
                                                         currentParent() );
        return mWaitingForChangeId != -1;
      }

    private Q_SLOTS:
      void onDeleteFinished( int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( changeId == mWaitingForChangeId ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            mLatestRevisionByItemId.remove( deletedIds.first() ); // TODO
          }
          emit finished( resultCode, errorString );
        }
      }

      void onCreateFinished( int changeId, const Akonadi::Item &item,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( changeId == mWaitingForChangeId ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            mLatestRevisionByItemId.insert( item.id(), item.revision() );
            mNewId = item.id();
          }
          emit finished( resultCode, errorString );
        }
      }
    private:
      Q_DISABLE_COPY(CreationEntry)
  };

  class DeletionEntry : public Entry {
    Q_OBJECT
    public:
      DeletionEntry( const Akonadi::Item &item, const QString &description,
                     History *q ) : Entry( item, description, q )
      {
        const Incidence::Ptr incidence = mItem.payload<KCalCore::Incidence::Ptr>();
        mDescription =  i18n( "%1 deletion",
                              KCalUtils::Stringify::incidenceType( incidence->type() ) );
        connect( mChanger, SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
                 SLOT(onCreateFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );
        connect( mChanger, SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
                 SLOT(onDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)) );
      }

      /**reimp*/
      bool undo()
      {
        // TODO: pass mCollection
        Q_ASSERT( mItem.hasPayload<KCalCore::Incidence::Ptr>() );
        mWaitingForChangeId = mChanger->createIncidence( mItem.payload<KCalCore::Incidence::Ptr>(),
                                                         Collection(),
                                                         currentParent() );
        return mWaitingForChangeId != -1;
      }

      /**reimp*/
      bool redo()
      {
        mWaitingForChangeId = mChanger->deleteIncidence( mItem, currentParent() );
        return mWaitingForChangeId != -1;
      }
    private Q_SLOTS:
      void onDeleteFinished( int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( changeId == mWaitingForChangeId ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            mLatestRevisionByItemId.remove( deletedIds.first() ); // TODO
          }
          emit finished( resultCode, errorString );
        }
      }
      void onCreateFinished( int changeId, const Akonadi::Item &item,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( changeId == mWaitingForChangeId ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            mNewId = item.id();
            mLatestRevisionByItemId.insert( item.id(), item.revision() );
          }
          emit finished( resultCode, errorString );
        }
      }
    private:
      Q_DISABLE_COPY(DeletionEntry)
  };

  class ModificationEntry : public Entry {
    Q_OBJECT
    public:
      ModificationEntry( const Akonadi::Item &item,
                         const Incidence::Ptr &originalPayload,
                         const QString &description,
                         History *q ) : Entry( item, description, q )
                                      , mOriginalPayload( originalPayload )
      {
        const Incidence::Ptr incidence = mItem.payload<KCalCore::Incidence::Ptr>();
        mDescription =  i18n( "%1 deletion",
                              KCalUtils::Stringify::incidenceType( incidence->type() ) );

        connect( mChanger,SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
                 SLOT(onModifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );
      }

      /**reimp*/
      bool undo()
      {
       Item oldItem = mItem;
       mItem.setPayload<KCalCore::Incidence::Ptr>( mOriginalPayload );
       mWaitingForChangeId = mChanger->modifyIncidence( oldItem, Incidence::Ptr(),
                                                       currentParent() );
        return mWaitingForChangeId != -1;
      }

      /**reimp*/
      bool redo()
      {
        mWaitingForChangeId = mChanger->modifyIncidence( mItem, mOriginalPayload,
                                                         currentParent() );
        return mWaitingForChangeId != -1;
      }
    private Q_SLOTS:
      void onModifyFinished( int changeId, const Akonadi::Item &item,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( changeId == mWaitingForChangeId ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            mLatestRevisionByItemId.insert( item.id(), item.revision() );
          }
          emit finished( resultCode, errorString );
        }
      }
    private:
      Q_DISABLE_COPY(ModificationEntry)
      Incidence::Ptr mOriginalPayload;
  };
}

#endif
