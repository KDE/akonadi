/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentbase.h"
#include "collection.h"
#include "item.h"
#include "resourcebase.h"

#include <QDBusMessage>
#include <QObject>

namespace Akonadi
{
class RecursiveMover;

/// @cond PRIVATE

/**
  @internal

  Manages synchronization and fetch requests for a resource.

  @todo Attach to the ResourceBase Monitor,
*/
class ResourceScheduler : public QObject
{
    Q_OBJECT

public:
    // If you change this enum, keep s_taskTypes in sync in resourcescheduler.cpp
    enum TaskType {
        Invalid,
        SyncAll,
        SyncCollectionTree,
        SyncCollection,
        SyncCollectionAttributes,
        SyncTags,
        FetchItem,
        FetchItems,
        ChangeReplay,
        RecursiveMoveReplay,
        DeleteResourceCollection,
        InvalideCacheForCollection,
        SyncAllDone,
        SyncCollectionTreeDone,
        SyncRelations,
        Custom
    };

    class Task
    {
        static qint64 latestSerial;

    public:
        Task()
            : serial(++latestSerial)
            , type(Invalid)
        {
        }
        qint64 serial;
        TaskType type;
        Collection collection;
        QVector<Item> items;
        QSet<QByteArray> itemParts;
        QList<QDBusMessage> dbusMsgs;
        QObject *receiver = nullptr;
        QByteArray methodName;
        QVariant argument;

        void sendDBusReplies(const QString &errorMsg);

        bool operator==(const Task &other) const
        {
            return type == other.type && (collection == other.collection || (!collection.isValid() && !other.collection.isValid())) && items == other.items
                && itemParts == other.itemParts && receiver == other.receiver && methodName == other.methodName && argument == other.argument;
        }
    };

    explicit ResourceScheduler(QObject *parent = nullptr);

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
    void scheduleSync(const Collection &col);

    /**
      Schedules synchronizing the attributes of a single collection.
      @param collection The collection to synchronize attributes from.
    */
    void scheduleAttributesSync(const Collection &collection);

    void scheduleTagSync();
    void scheduleRelationSync();

    /**
      Schedules fetching of a single PIM item.

      This task is only ever used if the resource still uses the old deprecated
      retrieveItem() (instead of retrieveItems(Item::List)) method. This task has
      a special meaning to the scheduler and instead of replying to the DBus message
      after the single @p item is retrieved, the items are accumulated until all
      tasks from the same messages are fetched.

      @param items The items to fetch.
      @param parts List of names of the parts of the item to fetch.
      @param msg The associated D-Bus message.
      @param parentId ID of the original ItemsFetch task that this task was created from.
                      We can use this ID to group the tasks together
    */
    void scheduleItemFetch(const Item &item, const QSet<QByteArray> &parts, const QList<QDBusMessage> &msgs, const qint64 parentId);

    /**
      Schedules batch-fetching of PIM items.
      @param items The items to fetch.
      @param parts List of names of the parts of the item to fetch.
      @param msg The associated D-Bus message.
    */
    void scheduleItemsFetch(const Item::List &item, const QSet<QByteArray> &parts, const QDBusMessage &msg);

    /**
      Schedules deletion of the resource collection.
      This method is used to implement the ResourceBase::clearCache() functionality.
     */
    void scheduleResourceCollectionDeletion();

    /**
     * Schedule cache invalidation for @p collection.
     * @see ResourceBase::invalidateCache()
     */
    void scheduleCacheInvalidation(const Collection &collection);

    /**
      Insert synchronization completion marker into the task queue.
    */
    void scheduleFullSyncCompletion();

    /**
      Insert collection tree synchronization completion marker into the task queue.
    */
    void scheduleCollectionTreeSyncCompletion();

    /**
      Insert a custom task.
      @param methodName The method name, without signature, do not use the SLOT() macro
    */
    void
    scheduleCustomTask(QObject *receiver, const char *methodName, const QVariant &argument, ResourceBase::SchedulePriority priority = ResourceBase::Append);

    /**
     * Schedule a recursive move replay.
     */
    void scheduleMoveReplay(const Collection &movedCollection, RecursiveMover *mover);

    /**
      Returns true if no tasks are running or in the queue.
    */
    bool isEmpty();

    /**
      Returns the current task.
    */
    Task currentTask() const;

    Task &currentTask();

    /**
      Sets the online state.
    */
    void setOnline(bool state);

    /**
       Print debug output showing the state of the scheduler.
    */
    void dump() const;
    /**
       Print debug output showing the state of the scheduler.
    */
    QString dumpToString() const;

    /**
       Clear the state of the scheduler. Warning: this is intended to be
       used purely in debugging scenarios, as it might cause loss of uncommitted
       local changes.
    */
    void clear();

    /**
       Cancel everything the scheduler has still in queue. Keep the current task running though.
       It can be seen as a less aggressive clear() used when the user requested the resource to
       abort its activities. It properly cancel all the tasks in there.
    */
    void cancelQueues();

public Q_SLOTS:
    /**
      Schedules replaying changes.
    */
    void scheduleChangeReplay();

    /**
      The current task has been finished
    */
    void taskDone();

    /**
      Like taskDone(), but special case for ItemFetch task
    */
    void itemFetchDone(const QString &msg);

    /**
      The current task can't be finished now and will be rescheduled later
    */
    void deferTask();

    /**
      Remove tasks that affect @p collection.
    */
    void collectionRemoved(const Akonadi::Collection &collection);

Q_SIGNALS:
    void executeFullSync();
    void executeCollectionAttributesSync(const Akonadi::Collection &col);
    void executeCollectionSync(const Akonadi::Collection &col);
    void executeCollectionTreeSync();
    void executeTagSync();
    void executeRelationSync();
    void executeItemFetch(const Akonadi::Item &item, const QSet<QByteArray> &parts);
    void executeItemsFetch(const QVector<Akonadi::Item> &items, const QSet<QByteArray> &parts);
    void executeResourceCollectionDeletion();
    void executeCacheInvalidation(const Akonadi::Collection &collection);
    void executeChangeReplay();
    void executeRecursiveMoveReplay(RecursiveMover *mover);
    void collectionTreeSyncComplete();
    void fullSyncComplete();
    void status(int status, const QString &message = QString());

private Q_SLOTS:
    void scheduleNext();
    void executeNext();

private:
    void signalTaskToTracker(const Task &task, const QByteArray &taskType, const QString &debugString = QString());

    // We have a number of task queues, by order of priority.
    // * PrependTaskQueue is for deferring the current task
    // * ChangeReplay must be first:
    //    change replays have to happen before we pull changes from the backend, otherwise
    //    we will overwrite our still unsaved local changes if the backend can't do
    //    incremental retrieval
    //
    // * then the stuff that is "immediately after change replay", like writeFile calls.
    // * then tasks which the user is waiting for, like ItemFetch (clicking on a mail) or
    //        SyncCollectionAttributes (folder properties dialog in kmail)
    // * then everything else (which includes the background email checking, which can take quite some time).
    enum QueueType {
        PrependTaskQueue,
        ChangeReplayQueue, // one task at most
        AfterChangeReplayQueue, // also one task at most, currently
        UserActionQueue,
        GenericTaskQueue,
        NQueueCount
    };
    using TaskList = QList<Task>;

    static QueueType queueTypeForTaskType(TaskType type);
    TaskList &queueForTaskType(TaskType type);

    TaskList mTaskList[NQueueCount];

    Task mCurrentTask;
    int mCurrentTasksQueue = -1; // queue mCurrentTask came from
    bool mOnline = false;
};

QDebug operator<<(QDebug, const ResourceScheduler::Task &task);
QTextStream &operator<<(QTextStream &, const ResourceScheduler::Task &task);

/// @endcond

}
