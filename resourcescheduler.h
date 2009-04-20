/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_RESOURCESCHEDULER_H
#define AKONADI_RESOURCESCHEDULER_H

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtDBus/QDBusMessage>

namespace Akonadi {

//@cond PRIVATE

/**
  @internal

  Manages synchronization and fetch requests for a resource.

  @todo Attach to the ResourceBase Monitor,
*/
class ResourceScheduler : public QObject
{
  Q_OBJECT

  public:
    enum TaskType {
      Invalid,
      SyncAll,
      SyncCollectionTree,
      SyncCollection,
      FetchItem,
      ChangeReplay,
      DeleteResourceCollection,
      SyncAllDone
    };

    class Task {
      static qint64 latestSerial;

      public:
        Task() : serial( ++latestSerial ), type( Invalid ) {}
        qint64 serial;
        TaskType type;
        Collection collection;
        Item item;
        QSet<QByteArray> itemParts;
        QDBusMessage dbusMsg;

        bool operator==( const Task &other ) const
        {
          return type == other.type
              && collection == other.collection
              && item == other.item
              && itemParts == other.itemParts;
        }
    };

    ResourceScheduler( QObject *parent = 0 );

    /**
      Schedules a full synchronization.
    */
    void scheduleFullSync();

    /**
      Schedules a collection tree sync.
    */
    void scheduleCollectionTreeSync();

    /**
      Schedules the synchronization of a single collection.
      @param col The collection to synchronize.
    */
    void scheduleSync( const Collection &col );

    /**
      Schedules fetching of a single PIM item.
      @param item The item to fetch.
      @param parts List of names of the parts of the item to fetch.
      @param msg The associated D-Bus message.
    */
    void scheduleItemFetch( const Item &item, const QSet<QByteArray> &parts, const QDBusMessage &msg );

    /**
      Schedules deletion of the resource collection.
      This method is used to implement the ResourceBase::clearCache() functionality.
     */
    void scheduleResourceCollectionDeletion();

    /**
      Insert synchronization completetion marker into the task queue.
    */
    void scheduleFullSyncCompletion();

    /**
      Returns true if no tasks are running or in the queue.
    */
    bool isEmpty();

    /**
      Returns the current task.
    */
    Task currentTask() const;

    /**
      Sets the online state.
    */
    void setOnline( bool state );

  public Q_SLOTS:
    /**
      Schedules replaying changes.
    */
    void scheduleChangeReplay();

    /**
      The current task has been finished
    */
    void taskDone();

  Q_SIGNALS:
    void executeFullSync();
    void executeCollectionSync( const Akonadi::Collection &col );
    void executeCollectionTreeSync();
    void executeItemFetch( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void executeResourceCollectionDeletion();
    void executeChangeReplay();
    void fullSyncComplete();
    void status( int status, const QString &message = QString() );

  private slots:
    void scheduleNext();
    void executeNext();

  private:
    void signalTaskToTracker( const Task &task, const QByteArray &taskType );

    QList<Task> mTaskList;
    Task mCurrentTask;
    bool mOnline;
};

//@endcond

}

#endif
