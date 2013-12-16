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

#ifndef AKONADI_HISTORY_P_H
#define AKONADI_HISTORY_P_H

#include "history.h"
#include "incidencechanger.h"
#include <kcalcore/incidence.h>
#include <akonadi/collection.h>

#include <QPointer>
#include <QStack>
#include <QVector>

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
  typedef QVector<Entry::Ptr> List;
  Entry( const Akonadi::Item &item, const QString &description, History *qq );
  Entry( const Akonadi::Item::List &items, const QString &description, History *qq );
  virtual void updateIds( Item::Id oldId, Item::Id newId );
  void doIt( OperationType );

  Akonadi::Item::List mItems;
  QString mDescription;
Q_SIGNALS:
  void finished( Akonadi::IncidenceChanger::ResultCode, const QString &errorString );
protected:
  virtual bool undo() = 0;
  virtual bool redo() = 0;
  void updateIdsGlobaly(Item::Id oldId, Item::Id newId);
  QWidget* currentParent() const;
  IncidenceChanger *mChanger;
  QHash<Akonadi::Item::Id,int> mLatestRevisionByItemId;
  History *q;
  QVector<int> mChangeIds;
private:
  void init( const QString &description, History *qq );
  Q_DISABLE_COPY(Entry)
};

class History::Private : public QObject {
  Q_OBJECT
public:
  Private( History *qq );
  ~Private(){}
  void doIt( OperationType );
  void stackEntry( const Entry::Ptr &entry, uint atomicOperationId );
  void updateIds( Item::Id oldId, Item::Id newId );
  void finishOperation( int changeId, History::ResultCode, const QString &errorString );
  QStack<Entry::Ptr>& destinationStack();
  QStack<Entry::Ptr>& stack( OperationType );
  QStack<Entry::Ptr>& stack();
  void undoOrRedo( OperationType, QWidget *parent );

  void emitDone( OperationType, History::ResultCode );
  void setEnabled( bool enabled );

  bool isUndoAvailable() const;
  bool isRedoAvailable() const;

  int redoCount() const;
  int undoCount() const;

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

class CreationEntry : public Entry {
  Q_OBJECT
public:
  typedef QSharedPointer<CreationEntry> Ptr;
  CreationEntry( const Akonadi::Item &item, const QString &description, History *q );

  /**reimp*/
  bool undo();

  /** reimp */
  bool redo();

private Q_SLOTS:
  void onDeleteFinished( int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                         Akonadi::IncidenceChanger::ResultCode resultCode,
                         const QString &errorString );

  void onCreateFinished( int changeId, const Akonadi::Item &item,
                         Akonadi::IncidenceChanger::ResultCode resultCode,
                         const QString &errorString );
private:
  Q_DISABLE_COPY(CreationEntry)
};

class DeletionEntry : public Entry {
  Q_OBJECT
public:
  DeletionEntry( const Akonadi::Item::List &items, const QString &description, History *q );
  /**reimp*/ bool undo();
  /**reimp*/ bool redo();

private Q_SLOTS:
  void onDeleteFinished( int changeId, const QVector<Akonadi::Item::Id> &deletedIds,
                         Akonadi::IncidenceChanger::ResultCode resultCode,
                         const QString &errorString );

  void onCreateFinished( int changeId, const Akonadi::Item &item,
                         Akonadi::IncidenceChanger::ResultCode resultCode,
                         const QString &errorString );
private:
  IncidenceChanger::ResultCode mResultCode;
  QString mErrorString;
  QHash<int,Akonadi::Item::Id> mOldIdByChangeId;
  int mNumPendingCreations;
  Q_DISABLE_COPY(DeletionEntry)
};

class ModificationEntry : public Entry {
  Q_OBJECT
public:
  ModificationEntry( const Akonadi::Item &item,
                      const Incidence::Ptr &originalPayload,
                      const QString &description,
                      History *q );

  /**reimp*/ bool undo();
  /**reimp*/ bool redo();

private Q_SLOTS:
  void onModifyFinished( int changeId, const Akonadi::Item &item,
                          Akonadi::IncidenceChanger::ResultCode resultCode,
                          const QString &errorString );
private:
  Q_DISABLE_COPY(ModificationEntry)
  Incidence::Ptr mOriginalPayload;
};

class MultiEntry : public Entry {
  Q_OBJECT
public:
    typedef QSharedPointer<MultiEntry> Ptr;
    MultiEntry( int id, const QString &description, History *q );

    void addEntry( const Entry::Ptr &entry );
    /** reimp */ void updateIds( Item::Id oldId, Item::Id newId );

protected:
    /**reimp*/ bool undo();
    /**reimp*/ bool redo();

private Q_SLOTS:
  void onEntryFinished( Akonadi::IncidenceChanger::ResultCode resultCode,
                          const QString &errorString );
public:
  const uint mAtomicOperationId;
private:
  Entry::List mEntries;
  int mFinishedEntries;
  OperationType mOperationInProgress;
  Q_DISABLE_COPY(MultiEntry)
};

}

#endif
