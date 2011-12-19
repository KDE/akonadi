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
      Entry( const Akonadi::Item::List &items, const QString &description, History *qq );
      virtual void updateIds( Item::Id oldId, Item::Id newId );
      void doIt( OperationType );

      Akonadi::Item::List mItems;
      QHash<Item::Id, Item::Id> mNewIdByOldId; // The change might result in a new ids
      QString mDescription;
    Q_SIGNALS:
      void finished( Akonadi::IncidenceChanger::ResultCode, const QString &errorString );
    protected:
      virtual bool undo() = 0;
      virtual bool redo() = 0;
      QWidget* currentParent() const;
      QSet<int> mChangeIdsToWaitFor;
      IncidenceChanger *mChanger;
      QHash<Akonadi::Item::Id,int> mLatestRevisionByItemId;
      History *q;
    private:
      void init( const QString &description, History *qq );
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

  Entry::Entry( const Akonadi::Item &item,
                const QString &description,
                History *qq ) : QObject()
  {
    mItems << item;
    init( description, qq );
  }

  Entry::Entry( const Akonadi::Item::List &items,
                const QString &description,
                History *qq ) : QObject(), mItems( items )
  {
    init( description, qq );
  }

  void Entry::init( const QString &description, History *qq )
  {
    mDescription = description;
    q = qq;
    mChanger = qq->d->mChanger;
  }

  QWidget* Entry::currentParent() const
  {
    return q->d->mCurrentParent;
  }

  void Entry::updateIds( Item::Id oldId, Item::Id newId )
  {
    Q_ASSERT( newId != -1 );
    Q_ASSERT( oldId != newId );

    Akonadi::Item::List::iterator it = mItems.begin();
    while ( it != mItems.end() ) {
      if ( (*it).id() == oldId )
        (*it).setId( newId );
      ++it;
    }
  }

  void Entry::doIt( OperationType type )
  {
    // We don't want incidence changer to re-record this stuff
    const bool oldHistoryEnabled = mChanger->historyEnabled();
    mChanger->setHistoryEnabled( false );
    mNewIdByOldId.clear();

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
        Q_ASSERT( mItems.count() == 1 );
        const Incidence::Ptr incidence = mItems.first().payload<KCalCore::Incidence::Ptr>();
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
        const int changeId = mChanger->deleteIncidence( mItems.first(), currentParent() );
        mChangeIdsToWaitFor << changeId;
        return changeId != -1;
      }

      bool redo()
      {
        // TODO: pass mCollection
        Akonadi::Item item = mItems.first();
        Q_ASSERT( item.hasPayload<KCalCore::Incidence::Ptr>() );
        const int changeId = mChanger->createIncidence( item.payload<KCalCore::Incidence::Ptr>(),
                                                        Collection(),
                                                        currentParent() );
        mChangeIdsToWaitFor << changeId;
        return changeId != -1;
      }

    private Q_SLOTS:
      void onDeleteFinished( int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( mChangeIdsToWaitFor.contains( changeId ) ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            Q_ASSERT( deletedIds.count() == 1 );
            mLatestRevisionByItemId.remove( deletedIds.first() ); // TODO
          }
          mChangeIdsToWaitFor.remove( changeId );
          emit finished( resultCode, errorString );
        }
      }

      void onCreateFinished( int changeId, const Akonadi::Item &item,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( mChangeIdsToWaitFor.contains( changeId ) ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            mLatestRevisionByItemId.insert( item.id(), item.revision() );
            mNewIdByOldId.clear();
            Q_ASSERT( mItems.count() == 1 );
            mNewIdByOldId.insert( mItems.first().id(), item.id() );
          }
          mChangeIdsToWaitFor.remove( changeId );
          emit finished( resultCode, errorString );
        }
      }
    private:
      Q_DISABLE_COPY(CreationEntry)
  };

  class DeletionEntry : public Entry {
    Q_OBJECT
    public:
      DeletionEntry( const Akonadi::Item::List &items, const QString &description,
                     History *q ) : Entry( items, description, q )
      {
        const Incidence::Ptr incidence = items.first().payload<KCalCore::Incidence::Ptr>();
        //TODO i18n
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
        mResultCode = IncidenceChanger::ResultCodeSuccess;
        mErrorString.clear();
        const bool useAtomicOperation = mItems.count() > 1 ;
        bool success = true;
        foreach( const Akonadi::Item &item, mItems ) {
          if ( useAtomicOperation )
            mChanger->startAtomicOperation();

          Q_ASSERT( item.hasPayload<KCalCore::Incidence::Ptr>() );
          const int changeId = mChanger->createIncidence( item.payload<KCalCore::Incidence::Ptr>(),
                                                          Collection(),
                                                          currentParent() );
          success = ( changeId != -1 ) && success;
          if ( useAtomicOperation )
            mChanger->endAtomicOperation();

          mChangeIdsToWaitFor << changeId;
          mOldIdByChangeId.insert( changeId, item.id() );
        }

        return success;
      }

      /**reimp*/
      bool redo()
      {
        const int changeId = mChanger->deleteIncidences( mItems, currentParent() );
        mChangeIdsToWaitFor << changeId;
        return changeId != -1;
      }
    private Q_SLOTS:
      void onDeleteFinished( int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( mChangeIdsToWaitFor.contains( changeId ) ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            foreach( Akonadi::Item::Id id, deletedIds )
              mLatestRevisionByItemId.remove( id ); // TODO
          }
          mChangeIdsToWaitFor.remove( changeId );
          emit finished( resultCode, errorString );
        }
      }

      void onCreateFinished( int changeId, const Akonadi::Item &item,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( mChangeIdsToWaitFor.contains( changeId ) ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            mNewIdByOldId.insert( mOldIdByChangeId.value( changeId ), item.id() );
            mLatestRevisionByItemId.insert( item.id(), item.revision() );
          } else {
            mResultCode = resultCode;
            mErrorString = errorString;
          }
          mChangeIdsToWaitFor.remove( changeId );
          mOldIdByChangeId.remove( changeId );
          if ( mChangeIdsToWaitFor.isEmpty() )
            emit finished( mResultCode, mErrorString );
        }
      }
    private:
      IncidenceChanger::ResultCode mResultCode;
      QString mErrorString;
      QHash<int,Akonadi::Item::Id> mOldIdByChangeId;
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
        const Incidence::Ptr incidence = mItems.first().payload<KCalCore::Incidence::Ptr>();
        mDescription =  i18n( "%1 deletion",
                              KCalUtils::Stringify::incidenceType( incidence->type() ) );

        connect( mChanger,SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
                 SLOT(onModifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );
      }

      /**reimp*/
      bool undo()
      {
        Item oldItem = mItems.first();
        oldItem.setPayload<KCalCore::Incidence::Ptr>( mOriginalPayload );
        const int changeId = mChanger->modifyIncidence( oldItem, Incidence::Ptr(), currentParent() );
        mChangeIdsToWaitFor << changeId;
        return changeId != -1;
      }

      /**reimp*/
      bool redo()
      {
        const int changeId = mChanger->modifyIncidence( mItems.first(), mOriginalPayload,
                                                        currentParent() );
        mChangeIdsToWaitFor << changeId;
        return changeId != -1;
      }
    private Q_SLOTS:
      void onModifyFinished( int changeId, const Akonadi::Item &item,
                             Akonadi::IncidenceChanger::ResultCode resultCode,
                             const QString &errorString )
      {
        if ( mChangeIdsToWaitFor.contains( changeId ) ) {
          if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
            mLatestRevisionByItemId.insert( item.id(), item.revision() );
          }
          mChangeIdsToWaitFor.remove( changeId );
          emit finished( resultCode, errorString );
        }
      }
    private:
      Q_DISABLE_COPY(ModificationEntry)
      Incidence::Ptr mOriginalPayload;
  };
}

#endif
