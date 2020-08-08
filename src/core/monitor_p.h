/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_MONITOR_P_H
#define AKONADI_MONITOR_P_H

#include "akonadicore_export.h"
#include "monitor.h"
#include "collection.h"
#include "collectionstatistics.h"
#include "collectionfetchscope.h"
#include "item.h"
#include "itemfetchscope.h"
#include "tagfetchscope.h"
#include "job.h"
#include "entitycache_p.h"
#include "servermanager.h"
#include "changenotificationdependenciesfactory_p.h"
#include "connection_p.h"
#include "commandbuffer_p.h"

#include "private/protocol_p.h"

#include <QObject>
#include <QTimer>

#include <QMimeDatabase>
#include <QMimeType>
#include <QPointer>

namespace Akonadi
{

class Monitor;
class ChangeNotification;

// A helper struct to wrap pointer to member function (which cannot be contained
// in a regular pointer)
struct SignalId {
    constexpr SignalId() = default;

    using Unit = uint;
    static constexpr int Size = sizeof(&Monitor::itemAdded) / sizeof(Unit);
    Unit data[sizeof(&Monitor::itemAdded) / sizeof(Unit)] = { 0 };

    inline bool operator==(SignalId other) const {
        for (int i = Size - 1; i >= 0; --i) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }
};

inline uint qHash(SignalId sig)
{
    // The 4 LSBs of the address should be enough to give us a good hash
    return sig.data[SignalId::Size - 1];
}

/**
 * @internal
 */
class AKONADICORE_EXPORT MonitorPrivate
{
public:
    enum ListenerAction {
        AddListener,
        RemoveListener
    };

    MonitorPrivate(ChangeNotificationDependenciesFactory *dependenciesFactory_, Monitor *parent);
    virtual ~MonitorPrivate();
    void init();

    Monitor *q_ptr;
    Q_DECLARE_PUBLIC(Monitor)
    ChangeNotificationDependenciesFactory *dependenciesFactory = nullptr;
    QPointer<Connection> ntfConnection;
    Collection::List collections;
    QSet<QByteArray> resources;
    QSet<Item::Id> items;
    QSet<Tag::Id> tags;
    QSet<Monitor::Type> types;
    QSet<QString> mimetypes;
    bool monitorAll;
    bool exclusive;
    QList<QByteArray> sessions;
    ItemFetchScope mItemFetchScope;
    TagFetchScope mTagFetchScope;
    CollectionFetchScope mCollectionFetchScope;
    bool mFetchChangedOnly;
    Session *session = nullptr;
    CollectionCache *collectionCache = nullptr;
    ItemListCache *itemCache = nullptr;
    TagListCache *tagCache = nullptr;
    QMimeDatabase mimeDatabase;
    QHash<SignalId, quint16> listeners;

    CommandBuffer mCommandBuffer;

    Protocol::ModifySubscriptionCommand::ModifiedParts pendingModificationChanges;
    Protocol::ModifySubscriptionCommand pendingModification;
    QTimer *pendingModificationTimer;
    bool monitorReady;

    // The waiting list
    QQueue<Protocol::ChangeNotificationPtr> pendingNotifications;
    // The messages for which data is currently being fetched
    QQueue<Protocol::ChangeNotificationPtr> pipeline;
    // In a pure Monitor, the pipeline contains items that were dequeued from pendingNotifications.
    // The ordering [pipeline] [pendingNotifications] is kept at all times.
    // [] [A B C]  -> [A B] [C]  -> [B] [C] -> [B C] [] -> [C] [] -> []
    // In a ChangeRecorder, the pipeline contains one item only, and not dequeued yet.
    // [] [A B C] -> [A] [A B C] -> [] [A B C] -> (changeProcessed) [] [B C] -> [B] [B C] etc...

    bool fetchCollection;
    bool fetchCollectionStatistics;
    bool collectionMoveTranslationEnabled;

    // Virtual methods for ChangeRecorder
    virtual void notificationsEnqueued(int)
    {
    }
    virtual void notificationsErased()
    {
    }

    // Virtual so it can be overridden in FakeMonitor.
    virtual bool connectToNotificationManager();
    void disconnectFromNotificationManager();

    void dispatchNotifications();
    void flushPipeline();

    bool ensureDataAvailable(const Protocol::ChangeNotificationPtr &msg);
    /**
     * Sends out the change notification @p msg.
     * @param msg the change notification to send
     * @return @c true if the notification was actually send to someone, @c false if no one was listening.
     */
    virtual bool emitNotification(const Protocol::ChangeNotificationPtr &msg);
    void updatePendingStatistics(const Protocol::ChangeNotificationPtr &msg);
    void invalidateCaches(const Protocol::ChangeNotificationPtr &msg);

    /** Used by ResourceBase to inform us about collection changes before the notifications are emitted,
        needed to avoid the missing RID race on change replay.
    */
    void invalidateCache(const Collection &col);

    /// Virtual so that ChangeRecorder can set it to 0 and handle the pipeline itself
    virtual int pipelineSize() const;

    // private Q_SLOTS
    void dataAvailable();
    void slotSessionDestroyed(QObject *object);
    void slotFlushRecentlyChangedCollections();

    /**
      Returns whether a message was appended to @p notificationQueue
    */
    int translateAndCompress(QQueue<Protocol::ChangeNotificationPtr> &notificationQueue, const Protocol::ChangeNotificationPtr &msg);

    void handleCommands();

    virtual void slotNotify(const Protocol::ChangeNotificationPtr &msg);

    /**
     * Sends out a change notification for an item.
     * @return @c true if the notification was actually send to someone, @c false if no one was listening.
     */
    bool emitItemsNotification(const Protocol::ItemChangeNotification &msg, const Item::List &items = Item::List(),
                               const Collection &collection = Collection(), const Collection &collectionDest = Collection());
    /**
     * Sends out a change notification for a collection.
     * @return @c true if the notification was actually send to someone, @c false if no one was listening.
     */
    bool emitCollectionNotification(const Protocol::CollectionChangeNotification &msg, const Collection &col = Collection(),
                                    const Collection &par = Collection(), const Collection &dest = Collection());

    bool emitTagNotification(const Protocol::TagChangeNotification &msg, const Tag &tags);

    bool emitRelationNotification(const Protocol::RelationChangeNotification &msg, const Relation &relation);

    bool emitSubscriptionChangeNotification(const Protocol::SubscriptionChangeNotification &msg,
                                            const NotificationSubscriber &subscriber);

    bool emitDebugChangeNotification(const Protocol::DebugChangeNotification &msg,
                                     const ChangeNotification &ntf);

    void serverStateChanged(Akonadi::ServerManager::State state);

    /**
     * This method is called by the ChangeMediator to enforce an invalidation of the passed collection.
     */
    void invalidateCollectionCache(qint64 collectionId);

    /**
     * This method is called by the ChangeMediator to enforce an invalidation of the passed item.
     */
    void invalidateItemCache(qint64 itemId);

    /**
     * This method is called by the ChangeMediator to enforce an invalidation of the passed tag.
     */
    void invalidateTagCache(qint64 tagId);

    void scheduleSubscriptionUpdate();
    void slotUpdateSubscription();

    void updateListeners(QMetaMethod signal, ListenerAction action);

    template<typename Signal>
    void updateListener(Signal signal, ListenerAction action)
    {
        auto it = listeners.find(signalId(signal));
        if (action == AddListener) {
            if (it == listeners.end()) {
                it = listeners.insert(signalId(signal), 0);
            }
            ++(*it);
        } else {
            if (--(*it) == 0) {
                listeners.erase(it);
            }
        }
    }

    static Protocol::ModifySubscriptionCommand::ChangeType monitorTypeToProtocol(Monitor::Type type);

    /**
      @brief Class used to determine when to purge items in a Collection

      The buffer method can be used to buffer a Collection. This may cause another Collection
      to be purged if it is removed from the buffer.

      The purge method is used to purge a Collection from the buffer, but not the model.
      This is used for example, to not buffer Collections anymore if they get referenced,
      and to ensure that one Collection does not appear twice in the buffer.

      Check whether a Collection is buffered using the isBuffered method.
    */
    class AKONADI_TESTS_EXPORT PurgeBuffer
    {
        // Buffer the most recent 10 unreferenced Collections
        static const int MAXBUFFERSIZE = 10;
    public:
        explicit PurgeBuffer()
        {
        }

        /**
          Adds @p id to the Collections to be buffered

          @returns The collection id which was removed form the buffer or -1 if none.
        */
        Collection::Id buffer(Collection::Id id);

        /**
        Removes @p id from the Collections being buffered
        */
        void purge(Collection::Id id);

        bool isBuffered(Collection::Id id) const
        {
            return m_buffer.contains(id);
        }

        static int buffersize();

    private:
        QQueue<Collection::Id> m_buffer;
    } m_buffer;

    QHash<Collection::Id, int> refCountMap;
    bool useRefCounting;
    void ref(Collection::Id id);
    Collection::Id deref(Collection::Id id);

    /**
     * Returns true if the collection is monitored by monitor.
     *
     * A collection is always monitored if useRefCounting is false.
     * If ref counting is used, the collection is only monitored,
     * if the collection is either in refCountMap or m_buffer.
     * If ref counting is used and the collection is not in refCountMap or m_buffer,
     * no updates for the contained items are emitted, because they are lazily ignored.
     */
    bool isMonitored(Collection::Id colId) const;

private:
    // collections that need a statistics update
    QSet<Collection::Id> recentlyChangedCollections;
    QTimer statisticsCompressionTimer;

    /**
      @returns True if @p msg should be ignored. Otherwise appropriate signals are emitted for it.
    */
    bool isLazilyIgnored(const Protocol::ChangeNotificationPtr &msg, bool allowModifyFlagsConversion = false) const;

    /**
      Sets @p needsSplit to True when @p msg contains more than one item and there's at least one
      listener that does not support batch operations. Sets @p batchSupported to True when
      there's at least one listener that supports batch operations.
    */
    void checkBatchSupport(const Protocol::ChangeNotificationPtr &msg, bool &needsSplit, bool &batchSupported) const;

    Protocol::ChangeNotificationList splitMessage(const Protocol::ItemChangeNotification &msg, bool legacy) const;

    bool isCollectionMonitored(Collection::Id collection) const
    {
        if (collection < 0) {
            return false;
        }
        if (collections.contains(Collection(collection))) {
            return true;
        }
        if (collections.contains(Collection::root())) {
            return true;
        }
        return false;
    }

    bool isMimeTypeMonitored(const QString &mimetype) const
    {
        if (mimetypes.contains(mimetype)) {
            return true;
        }

        const QMimeType mimeType = mimeDatabase.mimeTypeForName(mimetype);
        if (!mimeType.isValid()) {
            return false;
        }

        for (const QString &mt : mimetypes) {
            if (mimeType.inherits(mt)) {
                return true;
            }
        }

        return false;
    }

    template<typename T>
    bool isMoveDestinationResourceMonitored(const T &msg) const
    {
        if (msg.operation() != T::Move) {
            return false;
        }
        return resources.contains(msg.destinationResource());
    }

    void fetchStatistics(Collection::Id colId)
    {
        Akonadi::fetchCollectionStatistics(Collection{colId}, session).then(
                [this, colId](const CollectionStatistics &statistics) {
                    Q_EMIT q_ptr->collectionStatisticsChanged(colId, statistics);
                },
                [colId](const Error &error) {
                    qCWarning(AKONADICORE_LOG) << "Error while fetching statistics for collection" << colId << ":" << error.message();
                });
    }

    void notifyCollectionStatisticsWatchers(Collection::Id collection, const QByteArray &resource);
    bool fetchCollections() const;
    bool fetchItems() const;

    // A hack to "cast" pointer to member function to something we can easily
    // use as a key in the hashtable
    template<typename Signal>
    constexpr SignalId signalId(Signal signal) const
    {
        union {
            Signal in;
            SignalId out;
        } h = {signal};
        return h.out;
    }

    template<typename Signal>
    bool hasListeners(Signal signal) const
    {
        auto it = listeners.find(signalId(signal));
        return it != listeners.end();
    }

    template<typename Signal, typename ... Args>
    bool emitToListeners(Signal signal, Args ... args)
    {
        if (hasListeners(signal)) {
            Q_EMIT (q_ptr->*signal)(std::forward<Args>(args) ...);
            return true;
        }
        return false;
    }
};

}

#endif
