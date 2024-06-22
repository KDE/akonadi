/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "resourcescheduler_p.h"

#include "recursivemover_p.h"
#include <QDBusConnection>

#include "akonadiagentbase_debug.h"
#include "private/instance_p.h"
#include <KLocalizedString>

#include <QDBusInterface>
#include <QTimer>

using namespace Akonadi;
using namespace std::chrono_literals;
qint64 ResourceScheduler::Task::latestSerial = 0;
static QDBusAbstractInterface *s_resourcetracker = nullptr;

/// @cond PRIVATE

ResourceScheduler::ResourceScheduler(QObject *parent)
    : QObject(parent)
{
}

void ResourceScheduler::scheduleFullSync()
{
    Task t;
    t.type = SyncAll;
    TaskList &queue = queueForTaskType(t.type);
    if (queue.contains(t) || mCurrentTask == t) {
        return;
    }
    queue << t;
    signalTaskToTracker(t, "SyncAll");
    scheduleNext();
}

void ResourceScheduler::scheduleCollectionTreeSync()
{
    Task t;
    t.type = SyncCollectionTree;
    TaskList &queue = queueForTaskType(t.type);
    if (queue.contains(t) || mCurrentTask == t) {
        return;
    }
    queue << t;
    signalTaskToTracker(t, "SyncCollectionTree");
    scheduleNext();
}

void ResourceScheduler::scheduleTagSync()
{
    Task t;
    t.type = SyncTags;
    TaskList &queue = queueForTaskType(t.type);
    if (queue.contains(t) || mCurrentTask == t) {
        return;
    }
    queue << t;
    signalTaskToTracker(t, "SyncTags");
    scheduleNext();
}

void ResourceScheduler::scheduleSync(const Collection &col)
{
    Task t;
    t.type = SyncCollection;
    t.collection = col;
    TaskList &queue = queueForTaskType(t.type);
    if (queue.contains(t) || mCurrentTask == t) {
        return;
    }
    queue << t;
    signalTaskToTracker(t, "SyncCollection", QString::number(col.id()));
    scheduleNext();
}

void ResourceScheduler::scheduleAttributesSync(const Collection &collection)
{
    Task t;
    t.type = SyncCollectionAttributes;
    t.collection = collection;

    TaskList &queue = queueForTaskType(t.type);
    if (queue.contains(t) || mCurrentTask == t) {
        return;
    }
    queue << t;
    signalTaskToTracker(t, "SyncCollectionAttributes", QString::number(collection.id()));
    scheduleNext();
}

void ResourceScheduler::scheduleItemFetch(const Akonadi::Item &item, const QSet<QByteArray> &parts, const QList<QDBusMessage> &msgs, qint64 parentId)

{
    Task t;
    t.type = FetchItem;
    t.items << item;
    t.itemParts = parts;
    t.dbusMsgs = msgs;
    t.argument = parentId;

    TaskList &queue = queueForTaskType(t.type);
    queue << t;

    signalTaskToTracker(t, "FetchItem", QString::number(item.id()));
    scheduleNext();
}

void ResourceScheduler::scheduleItemsFetch(const Item::List &items, const QSet<QByteArray> &parts, const QDBusMessage &msg)
{
    Task t;
    t.type = FetchItems;
    t.items = items;
    t.itemParts = parts;

    // if the current task does already fetch the requested item, break here but
    // keep the dbus message, so we can send the reply later on
    if (mCurrentTask == t) {
        mCurrentTask.dbusMsgs << msg;
        return;
    }

    // If this task is already in the queue, merge with it.
    TaskList &queue = queueForTaskType(t.type);
    const int idx = queue.indexOf(t);
    if (idx != -1) {
        queue[idx].dbusMsgs << msg;
        return;
    }

    t.dbusMsgs << msg;
    queue << t;

    QStringList ids;
    ids.reserve(items.size());
    for (const auto &item : items) {
        ids.push_back(QString::number(item.id()));
    }
    signalTaskToTracker(t, "FetchItems", ids.join(QLatin1StringView(", ")));
    scheduleNext();
}

void ResourceScheduler::scheduleResourceCollectionDeletion()
{
    Task t;
    t.type = DeleteResourceCollection;
    TaskList &queue = queueForTaskType(t.type);
    if (queue.contains(t) || mCurrentTask == t) {
        return;
    }
    queue << t;
    signalTaskToTracker(t, "DeleteResourceCollection");
    scheduleNext();
}

void ResourceScheduler::scheduleCacheInvalidation(const Collection &collection)
{
    Task t;
    t.type = InvalideCacheForCollection;
    t.collection = collection;
    TaskList &queue = queueForTaskType(t.type);
    if (queue.contains(t) || mCurrentTask == t) {
        return;
    }
    queue << t;
    signalTaskToTracker(t, "InvalideCacheForCollection", QString::number(collection.id()));
    scheduleNext();
}

void ResourceScheduler::scheduleChangeReplay()
{
    Task t;
    t.type = ChangeReplay;
    TaskList &queue = queueForTaskType(t.type);
    // see ResourceBase::changeProcessed() for why we do not check for mCurrentTask == t here like in the other tasks
    if (queue.contains(t)) {
        return;
    }
    queue << t;
    signalTaskToTracker(t, "ChangeReplay");
    scheduleNext();
}

void ResourceScheduler::scheduleMoveReplay(const Collection &movedCollection, RecursiveMover *mover)
{
    Task t;
    t.type = RecursiveMoveReplay;
    t.collection = movedCollection;
    t.argument = QVariant::fromValue(mover);
    TaskList &queue = queueForTaskType(t.type);

    if (queue.contains(t) || mCurrentTask == t) {
        return;
    }

    queue << t;
    signalTaskToTracker(t, "RecursiveMoveReplay", QString::number(t.collection.id()));
    scheduleNext();
}

void Akonadi::ResourceScheduler::scheduleFullSyncCompletion()
{
    Task t;
    t.type = SyncAllDone;
    TaskList &queue = queueForTaskType(t.type);
    // no compression here, all this does is emitting a D-Bus signal anyway, and compression can trigger races on the receiver side with the signal being lost
    queue << t;
    signalTaskToTracker(t, "SyncAllDone");
    scheduleNext();
}

void Akonadi::ResourceScheduler::scheduleCollectionTreeSyncCompletion()
{
    Task t;
    t.type = SyncCollectionTreeDone;
    TaskList &queue = queueForTaskType(t.type);
    // no compression here, all this does is emitting a D-Bus signal anyway, and compression can trigger races on the receiver side with the signal being lost
    queue << t;
    signalTaskToTracker(t, "SyncCollectionTreeDone");
    scheduleNext();
}

void Akonadi::ResourceScheduler::scheduleCustomTask(QObject *receiver,
                                                    const char *methodName,
                                                    const QVariant &argument,
                                                    ResourceBase::SchedulePriority priority)
{
    Task t;
    t.type = Custom;
    t.receiver = receiver;
    t.methodName = methodName;
    t.argument = argument;
    QueueType queueType = GenericTaskQueue;
    if (priority == ResourceBase::AfterChangeReplay) {
        queueType = AfterChangeReplayQueue;
    } else if (priority == ResourceBase::Prepend) {
        queueType = PrependTaskQueue;
    }
    TaskList &queue = mTaskList[queueType];

    if (queue.contains(t)) {
        return;
    }

    switch (priority) {
    case ResourceBase::Prepend:
        queue.prepend(t);
        break;
    default:
        queue.append(t);
        break;
    }

    signalTaskToTracker(t, "Custom-" + t.methodName);
    scheduleNext();
}

void ResourceScheduler::taskDone()
{
    if (isEmpty()) {
        Q_EMIT status(AgentBase::Idle, i18nc("@info:status Application ready for work", "Ready"));
    }

    if (s_resourcetracker) {
        const QList<QVariant> argumentList = {QString::number(mCurrentTask.serial), QString()};
        s_resourcetracker->asyncCallWithArgumentList(QStringLiteral("jobEnded"), argumentList);
    }

    mCurrentTask = Task();
    mCurrentTasksQueue = -1;
    scheduleNext();
}

void ResourceScheduler::itemFetchDone(const QString &msg)
{
    Q_ASSERT(mCurrentTask.type == FetchItem);

    TaskList &queue = queueForTaskType(mCurrentTask.type);

    const qint64 parentId = mCurrentTask.argument.toLongLong();
    // msg is empty, there was no error
    if (msg.isEmpty() && !queue.isEmpty()) {
        Task &nextTask = queue[0];
        // If the next task is FetchItem too...
        if (nextTask.type != mCurrentTask.type || nextTask.argument.toLongLong() != parentId) {
            // If the next task is not FetchItem or the next FetchItem task has
            // different parentId then this was the last task in the series, so
            // send the DBus replies.
            mCurrentTask.sendDBusReplies(msg);
        }
    } else {
        // msg was not empty, there was an error.
        // remove all subsequent FetchItem tasks with the same parentId
        auto iter = queue.begin();
        while (iter != queue.end()) {
            if (iter->type != mCurrentTask.type || iter->argument.toLongLong() == parentId) {
                iter = queue.erase(iter);
                continue;
            } else {
                break;
            }
        }

        // ... and send DBus reply with the error message
        mCurrentTask.sendDBusReplies(msg);
    }

    taskDone();
}

void ResourceScheduler::deferTask()
{
    if (mCurrentTask.type == Invalid) {
        return;
    }

    if (s_resourcetracker) {
        const QList<QVariant> argumentList = {QString::number(mCurrentTask.serial), QString()};
        s_resourcetracker->asyncCallWithArgumentList(QStringLiteral("jobEnded"), argumentList);
    }

    Task t = mCurrentTask;
    mCurrentTask = Task();

    Q_ASSERT(mCurrentTasksQueue >= 0 && mCurrentTasksQueue < NQueueCount);
    mTaskList[mCurrentTasksQueue].prepend(t);
    mCurrentTasksQueue = -1;

    signalTaskToTracker(t, "DeferedTask");

    scheduleNext();
}

bool ResourceScheduler::isEmpty()
{
    for (int i = 0; i < NQueueCount; ++i) {
        if (!mTaskList[i].isEmpty()) {
            return false;
        }
    }
    return true;
}

void ResourceScheduler::scheduleNext()
{
    if (mCurrentTask.type != Invalid || isEmpty() || !mOnline) {
        return;
    }
    QTimer::singleShot(0s, this, &ResourceScheduler::executeNext);
}

void ResourceScheduler::executeNext()
{
    if (mCurrentTask.type != Invalid || isEmpty()) {
        return;
    }

    for (int i = 0; i < NQueueCount; ++i) {
        if (!mTaskList[i].isEmpty()) {
            mCurrentTask = mTaskList[i].takeFirst();
            mCurrentTasksQueue = i;
            break;
        }
    }

    if (s_resourcetracker) {
        const QList<QVariant> argumentList = {QString::number(mCurrentTask.serial)};
        s_resourcetracker->asyncCallWithArgumentList(QStringLiteral("jobStarted"), argumentList);
    }

    switch (mCurrentTask.type) {
    case SyncAll:
        Q_EMIT executeFullSync();
        break;
    case SyncCollectionTree:
        Q_EMIT executeCollectionTreeSync();
        break;
    case SyncCollection:
        Q_EMIT executeCollectionSync(mCurrentTask.collection);
        break;
    case SyncCollectionAttributes:
        Q_EMIT executeCollectionAttributesSync(mCurrentTask.collection);
        break;
    case SyncTags:
        Q_EMIT executeTagSync();
        break;
    case FetchItem:
        Q_EMIT executeItemFetch(mCurrentTask.items.at(0), mCurrentTask.itemParts);
        break;
    case FetchItems:
        Q_EMIT executeItemsFetch(mCurrentTask.items, mCurrentTask.itemParts);
        break;
    case DeleteResourceCollection:
        Q_EMIT executeResourceCollectionDeletion();
        break;
    case InvalideCacheForCollection:
        Q_EMIT executeCacheInvalidation(mCurrentTask.collection);
        break;
    case ChangeReplay:
        Q_EMIT executeChangeReplay();
        break;
    case RecursiveMoveReplay:
        Q_EMIT executeRecursiveMoveReplay(mCurrentTask.argument.value<RecursiveMover *>());
        break;
    case SyncAllDone:
        Q_EMIT fullSyncComplete();
        break;
    case SyncCollectionTreeDone:
        Q_EMIT collectionTreeSyncComplete();
        break;
    case Custom: {
        const QByteArray methodSig = mCurrentTask.methodName + QByteArray("(QVariant)");
        const bool hasSlotWithVariant = mCurrentTask.receiver->metaObject()->indexOfMethod(methodSig.constData()) != -1;
        bool success = false;
        if (hasSlotWithVariant) {
            success = QMetaObject::invokeMethod(mCurrentTask.receiver, mCurrentTask.methodName.constData(), Q_ARG(QVariant, mCurrentTask.argument));
            Q_ASSERT_X(success || !mCurrentTask.argument.isValid(),
                       "ResourceScheduler::executeNext",
                       "Valid argument was provided but the method wasn't found");
        }
        if (!success) {
            success = QMetaObject::invokeMethod(mCurrentTask.receiver, mCurrentTask.methodName.constData());
        }

        if (!success) {
            qCCritical(AKONADIAGENTBASE_LOG) << "Could not invoke slot" << mCurrentTask.methodName << "on" << mCurrentTask.receiver << "with argument"
                                             << mCurrentTask.argument;
        }
        break;
    }
    default: {
        qCCritical(AKONADIAGENTBASE_LOG) << "Unhandled task type" << mCurrentTask.type;
        dump();
        Q_ASSERT(false);
    }
    }
}

ResourceScheduler::Task ResourceScheduler::currentTask() const
{
    return mCurrentTask;
}

ResourceScheduler::Task &ResourceScheduler::currentTask()
{
    return mCurrentTask;
}

void ResourceScheduler::setOnline(bool state)
{
    if (mOnline == state) {
        return;
    }
    mOnline = state;
    if (mOnline) {
        scheduleNext();
    } else {
        if (mCurrentTask.type != Invalid) {
            // abort running task
            queueForTaskType(mCurrentTask.type).prepend(mCurrentTask);
            mCurrentTask = Task();
            mCurrentTasksQueue = -1;
        }
        // abort pending synchronous tasks, might take longer until the resource goes online again
        TaskList &itemFetchQueue = queueForTaskType(FetchItem);
        qint64 parentId = -1;
        Task lastTask;
        for (QList<Task>::iterator it = itemFetchQueue.begin(); it != itemFetchQueue.end();) {
            if ((*it).type == FetchItem) {
                qint64 idx = it->argument.toLongLong();
                if (parentId == -1) {
                    parentId = idx;
                }
                if (idx != parentId) {
                    // Only emit the DBus reply once we reach the last taskwith the
                    // same "idx"
                    lastTask.sendDBusReplies(i18nc("@info", "Job canceled."));
                    parentId = idx;
                }
                lastTask = (*it);
                it = itemFetchQueue.erase(it);
                if (s_resourcetracker) {
                    const QList<QVariant> argumentList = {QString::number(mCurrentTask.serial), i18nc("@info", "Job canceled.")};
                    s_resourcetracker->asyncCallWithArgumentList(QStringLiteral("jobEnded"), argumentList);
                }
            } else {
                ++it;
            }
        }
    }
}

void ResourceScheduler::signalTaskToTracker(const Task &task, const QByteArray &taskType, const QString &debugString)
{
    // if there's a job tracer running, tell it about the new job
    if (!s_resourcetracker) {
        const QString suffix = Akonadi::Instance::identifier().isEmpty() ? QString() : QLatin1Char('-') + Akonadi::Instance::identifier();
        if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.akonadiconsole") + suffix)) {
            s_resourcetracker = new QDBusInterface(QLatin1StringView("org.kde.akonadiconsole") + suffix,
                                                   QStringLiteral("/resourcesJobtracker"),
                                                   QStringLiteral("org.freedesktop.Akonadi.JobTracker"),
                                                   QDBusConnection::sessionBus(),
                                                   nullptr);
        }
    }

    if (s_resourcetracker) {
        const QList<QVariant> argumentList = QList<QVariant>() << static_cast<AgentBase *>(parent())->identifier() // "session" (in our case resource)
                                                               << QString::number(task.serial) // "job"
                                                               << QString() // "parent job"
                                                               << QString::fromLatin1(taskType) // "job type"
                                                               << debugString // "job debugging string"
            ;
        s_resourcetracker->asyncCallWithArgumentList(QStringLiteral("jobCreated"), argumentList);
    }
}

void ResourceScheduler::collectionRemoved(const Akonadi::Collection &collection)
{
    if (!collection.isValid()) { // should not happen, but you never know...
        return;
    }
    TaskList &queue = queueForTaskType(SyncCollection);
    for (QList<Task>::iterator it = queue.begin(); it != queue.end();) {
        if ((*it).type == SyncCollection && (*it).collection == collection) {
            it = queue.erase(it);
            qCDebug(AKONADIAGENTBASE_LOG) << " erasing";
        } else {
            ++it;
        }
    }
}

void ResourceScheduler::Task::sendDBusReplies(const QString &errorMsg)
{
    for (const QDBusMessage &msg : std::as_const(dbusMsgs)) {
        qCDebug(AKONADIAGENTBASE_LOG) << "Sending dbus reply for method" << methodName << "with error" << errorMsg;
        QDBusMessage reply;
        if (!errorMsg.isEmpty()) {
            reply = msg.createErrorReply(QDBusError::Failed, errorMsg);
        } else if (msg.member() == QLatin1StringView("requestItemDelivery")) {
            reply = msg.createReply();
        } else if (msg.member().isEmpty()) {
            continue; // unittest calls scheduleItemFetch with empty QDBusMessage
        } else {
            qCCritical(AKONADIAGENTBASE_LOG) << "ResourceScheduler: got unexpected method name :" << msg.member();
        }
        QDBusConnection::sessionBus().send(reply);
    }
}

ResourceScheduler::QueueType ResourceScheduler::queueTypeForTaskType(TaskType type)
{
    switch (type) {
    case ChangeReplay:
    case RecursiveMoveReplay:
        return ChangeReplayQueue;
    case FetchItem:
    case FetchItems:
    case SyncCollectionAttributes:
        return UserActionQueue;
    default:
        return GenericTaskQueue;
    }
}

ResourceScheduler::TaskList &ResourceScheduler::queueForTaskType(TaskType type)
{
    const QueueType qt = queueTypeForTaskType(type);
    return mTaskList[qt];
}

void ResourceScheduler::dump() const
{
    qCDebug(AKONADIAGENTBASE_LOG) << dumpToString();
}

QString ResourceScheduler::dumpToString() const
{
    QString ret;
    QTextStream str(&ret);
    str << "ResourceScheduler: " << (mOnline ? "Online" : "Offline") << '\n';
    str << " current task: " << mCurrentTask << '\n';
    for (int i = 0; i < NQueueCount; ++i) {
        const TaskList &queue = mTaskList[i];
        if (queue.isEmpty()) {
            str << " queue " << i << " is empty" << '\n';
        } else {
            str << " queue " << i << " " << queue.size() << " tasks:\n";
            const QList<Task>::const_iterator queueEnd(queue.constEnd());
            for (QList<Task>::const_iterator it = queue.constBegin(); it != queueEnd; ++it) {
                str << "  " << (*it) << '\n';
            }
        }
    }
    str.flush();
    return ret;
}

void ResourceScheduler::clear()
{
    qCDebug(AKONADIAGENTBASE_LOG) << "Clearing ResourceScheduler queues:";
    for (int i = 0; i < NQueueCount; ++i) {
        TaskList &queue = mTaskList[i];
        queue.clear();
    }
    mCurrentTask = Task();
    mCurrentTasksQueue = -1;
}

void Akonadi::ResourceScheduler::cancelQueues()
{
    for (int i = 0; i < NQueueCount; ++i) {
        TaskList &queue = mTaskList[i];
        if (s_resourcetracker) {
            for (const Task &t : queue) {
                QList<QVariant> argumentList{QString::number(t.serial), QString()};
                s_resourcetracker->asyncCallWithArgumentList(QStringLiteral("jobEnded"), argumentList);
            }
        }
        queue.clear();
    }
}

static const char s_taskTypes[][27] = {"Invalid (no task)",
                                       "SyncAll",
                                       "SyncCollectionTree",
                                       "SyncCollection",
                                       "SyncCollectionAttributes",
                                       "SyncTags",
                                       "FetchItem",
                                       "FetchItems",
                                       "ChangeReplay",
                                       "RecursiveMoveReplay",
                                       "DeleteResourceCollection",
                                       "InvalideCacheForCollection",
                                       "SyncAllDone",
                                       "SyncCollectionTreeDone",
                                       "Custom"};

QTextStream &Akonadi::operator<<(QTextStream &d, const ResourceScheduler::Task &task)
{
    d << task.serial << " " << s_taskTypes[task.type] << " ";
    if (task.type != ResourceScheduler::Invalid) {
        if (task.collection.isValid()) {
            d << "collection " << task.collection.id() << " ";
        }
        if (!task.items.isEmpty()) {
            QStringList ids;
            ids.reserve(task.items.size());
            for (const auto &item : std::as_const(task.items)) {
                ids.push_back(QString::number(item.id()));
            }
            d << "items " << ids.join(QLatin1StringView(", ")) << " ";
        }
        if (!task.methodName.isEmpty()) {
            d << task.methodName << " " << task.argument.toString();
        }
    }
    return d;
}

QDebug Akonadi::operator<<(QDebug d, const ResourceScheduler::Task &task)
{
    QString s;
    QTextStream str(&s);
    str << task;
    d << s;
    return d;
}

/// @endcond

#include "moc_resourcescheduler_p.cpp"
