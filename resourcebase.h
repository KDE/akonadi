/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>
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

#ifndef AKONADI_RESOURCEBASE_H
#define AKONADI_RESOURCEBASE_H

#include "akonadi_export.h"

#include <akonadi/agentbase.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/itemsync.h>

class KJob;
class Akonadi__ResourceAdaptor;
class ResourceState;

namespace Akonadi {

class ResourceBasePrivate;

/**
 * @short The base class for all Akonadi resources.
 *
 * This class should be used as a base class by all resource agents,
 * because it encapsulates large parts of the protocol between
 * resource agent, agent manager and the Akonadi storage.
 *
 * It provides many convenience methods to make implementing a
 * new Akonadi resource agent as simple as possible.
 *
 * <h4>How to write a resource</h4>
 *
 * The following provides an overview of what you need to do to implement
 * your own Akonadi resource. In the following, the term 'backend' refers
 * to the entity the resource connects with Akonadi, be it a single file
 * or a remote server.
 *
 * @todo Complete this (online/offline state management)
 *
 * <h5>Basic %Resource Framework</h5>
 *
 * The following is needed to create a new resource:
 * - A new class deriving from Akonadi::ResourceBase, implementing at least all
 *   pure-virtual methods, see below for further details.
 * - call init() in your main() function.
 * - a .desktop file similar to the following example
 *   \code
 * [Desktop Entry]
 * Encoding=UTF-8
 * Name=My Akonadi Resource
 * Type=AkonadiResource
 * Exec=akonadi_my_resource
 * Icon=my-icon
 *
 * X-Akonadi-MimeTypes=<supported-mimetypes>
 * X-Akonadi-Capabilities=Resource
 * X-Akonadi-Identifier=akonadi_my_resource
 *   \endcode
 *
 * <h5>Handling PIM Items</h5>
 *
 * To follow item changes in the backend, the following steps are necessary:
 * - Implement retrieveItems() to synchronize all items in the given
 *   collection. If the backend supports incremental retrieval,
 *   implementing support for that is recommended to improve performance.
 * - Convert the items provided by the backend to Akonadi items.
 *   This typically happens either in retrieveItems() if you retrieved
 *   the collection synchronously (not recommended for network backends) or
 *   in the result slot of the asynchronous retrieval job.
 *   Converting means to create Akonadi::Item objects for every retrieved
 *   item. It's very important that every object has its remote identifier set.
 * - Call itemsRetrieved() or itemsRetrievedIncremental() respectively
 *   with the item objects created above. The Akonadi storage will then be
 *   updated automatically. Note that it is usually not necessary to manipulate
 *   any item in the Akonadi storage manually.
 *
 * To fetch item data on demand, the method retrieveItem() needs to be
 * reimplemented. Fetch the requested data there and call itemRetrieved()
 * with the result item.
 *
 * To write local changes back to the backend, you need to re-implement
 * the following three methods:
 * - itemAdded()
 * - itemChanged()
 * - itemRemoved()
 *
 * Note that these three functions don't get the full payload of the items by default,
 * you need to change the item fetch scope of the change recorder to fetch the full
 * payload. This can be expensive with big payloads, though.<br>
 * Once you have handled changes in these methods, call changeCommitted().
 * These methods are called whenever a local item related to this resource is
 * added, modified or deleted. They are only called if the resource is online, otherwise
 * all changes are recorded and replayed as soon the resource is online again.
 *
 * <h5>Handling Collections</h5>
 *
 * To follow collection changes in the backend, the following steps are necessary:
 * - Implement retrieveCollections() to retrieve collections from the backend.
 *   If the backend supports incremental collections updates, implementing
 *   support for that is recommended to improve performance.
 * - Convert the collections of the backend to Akonadi collections.
 *   This typically happens either in retrieveCollections() if you retrieved
 *   the collection synchronously (not recommended for network backends) or
 *   in the result slot of the asynchronous retrieval job.
 *   Converting means to create Akonadi::Collection objects for every retrieved
 *   collection. It's very important that every object has its remote identifier
 *   and its parent remote identifier set.
 * - Call collectionsRetrieved() or collectionsRetrievedIncremental() respectively
 *   with the collection objects created above. The Akonadi storage will then be
 *   updated automatically. Note that it is usually not necessary to manipulate
 *   any collection in the Akonadi storage manually.
 *
 *
 * To write local collection changes back to the backend, you need to re-implement
 * the following three methods:
 * - collectionAdded()
 * - collectionChanged()
 * - collectionRemoved()
 * Once you have handled changes in these methods call changeCommitted().
 * These methods are called whenever a local collection related to this resource is
 * added, modified or deleted. They are only called if the resource is online, otherwise
 * all changes are recorded and replayed as soon the resource is online again.
 *
 * @todo Convenience base class for collection-less resources
 */
// FIXME_API: API dox need to be updated for Observer approach (kevin)
class AKONADI_EXPORT ResourceBase : public AgentBase
{
    Q_OBJECT

public:
    /**
     * Use this method in the main function of your resource
     * application to initialize your resource subclass.
     * This method also takes care of creating a KApplication
     * object and parsing command line arguments.
     *
     * @note In case the given class is also derived from AgentBase::Observer
     *       it gets registered as its own observer (see AgentBase::Observer), e.g.
     *       <tt>resourceInstance->registerObserver( resourceInstance );</tt>
     *
     * @code
     *
     *   class MyResource : public ResourceBase
     *   {
     *     ...
     *   };
     *
     *   int main( int argc, char **argv )
     *   {
     *     return ResourceBase::init<MyResource>( argc, argv );
     *   }
     *
     * @endcode
     *
     * @param argc number of arguments
     * @param argv string arguments
     */
    template <typename T>
    static int init(int argc, char **argv)
    {
        const QString id = parseArguments(argc, argv);
        KApplication app;
        T *r = new T(id);

        // check if T also inherits AgentBase::Observer and
        // if it does, automatically register it on itself
        Observer *observer = dynamic_cast<Observer *>(r);
        if (observer != 0) {
            r->registerObserver(observer);
        }

        return init(r);
    }

    /**
     * This method is used to set the name of the resource.
     */
    void setName(const QString &name);

    /**
     * Returns the name of the resource.
     */
    QString name() const;

    /**
     * Enable or disable automatic progress reporting. By default, it is enabled.
     * When enabled, the resource will automatically emit the signals percent() and status()
     * while syncing items or collections.
     *
     * The automatic progress reporting is done on a per item / per collection basis, so if a
     * finer granularity is desired, automatic reporting should be disabled and the subclass should
     * emit the percent() and status() signals itself.
     *
     * @param enabled Whether or not automatic emission of the signals is enabled.
     * @since 4.7
     */
    void setAutomaticProgressReporting(bool enabled);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the name of the resource has changed.
     *
     * @param name The new name of the resource.
     */
    void nameChanged(const QString &name);

    /**
     * Emitted when a full synchronization has been completed.
     */
    void synchronized();

    /**
     * Emitted when a collection attributes synchronization has been completed.
     *
     * @param collectionId The identifier of the collection whose attributes got synchronized.
     * @since 4.6
     */
    void attributesSynchronized(qlonglong collectionId);

    /**
     * Emitted when a collection tree synchronization has been completed.
     *
     * @since 4.8
     */
    void collectionTreeSynchronized();

    /**
     * Emitted when the item synchronization processed the current batch and is ready for a new one.
     * Use this to throttle the delivery to not overload Akonadi.
     *
     * Throttling can be used during item retrieval (retrieveItems(Akonadi::Collection)) in streaming mode.
     * To throttle only deliver itemSyncBatchSize() items, and wait for this signal, then again deliver
     * @param remainingBatchSize items.
     *
     * By always only providing the number of items required to process the batch, the items don't pile
     * up in memory and items are only provided as fast as Akonadi can process them.
     *
     * @see batchSize()
     *
     * @since 4.14
     */
    void retrieveNextItemSyncBatch(int remainingBatchSize);

protected Q_SLOTS:
    /**
     * Retrieve the collection tree from the backend and supply it via
     * collectionsRetrieved() or collectionsRetrievedIncremental().
     * @see collectionsRetrieved(), collectionsRetrievedIncremental()
     */
    virtual void retrieveCollections() = 0;

    /**
     * Retreive all tags from the backend
     * @see tagsRetrieved()
     */
    virtual void retrieveTags();

    /**
     * Retrieve the attributes of a single collection from the backend. The
     * collection to retrieve attributes for is provided as @p collection.
     * Add the attributes parts and call collectionAttributesRetrieved()
     * when done.
     *
     * @param collection The collection whose attributes should be retrieved.
     * @see collectionAttributesRetrieved()
     * @since 4.6
     */
    // KDE5: Make it pure virtual, for now can be called only by invokeMethod()
    //       in order to simulate polymorphism
    void retrieveCollectionAttributes(const Akonadi::Collection &collection);

    /**
     * Retrieve all (new/changed) items in collection @p collection.
     * It is recommended to use incremental retrieval if the backend supports that
     * and provide the result by calling itemsRetrievedIncremental().
     * If incremental retrieval is not possible, provide the full listing by calling
     * itemsRetrieved( const Item::List& ).
     * In any case, ensure that all items have a correctly set remote identifier
     * to allow synchronizing with items already existing locally.
     * In case you don't want to use the built-in item syncing code, store the retrieved
     * items manually and call itemsRetrieved() once you are done.
     * @param collection The collection whose items to retrieve.
     * @see itemsRetrieved( const Item::List& ), itemsRetrievedIncremental(), itemsRetrieved(), currentCollection(), batchSize()
     */
    virtual void retrieveItems(const Akonadi::Collection &collection) = 0;

    /**
     * Returns the batch size used during the item sync.
     *
     * This can be used to throttle the item delivery.
     *
     * @see retrieveNextItemSyncBatch(int), retrieveItems(Akonadi::Collection)
     * @since 4.14
     */
    int itemSyncBatchSize() const;

    /**
     * Set the batch size used during the item sync.
     * The default is 10.
     *
     * @see retrieveNextItemSyncBatch(int)
     * @since 4.14
     */
    void setItemSyncBatchSize(int batchSize);

    /**
     * Retrieve a single item from the backend. The item to retrieve is provided as @p item.
     * Add the requested payload parts and call itemRetrieved() when done.
     * @param item The empty item whose payload should be retrieved. Use this object when delivering
     * the result instead of creating a new item to ensure conflict detection will work.
     * @param parts The item parts that should be retrieved.
     * @return false if there is an immediate error when retrieving the item.
     * @see itemRetrieved()
     */
    virtual bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) = 0;

    /**
     * Abort any activity in progress in the backend. By default this method does nothing.
     *
     * @since 4.6
     */
    // KDE5: Make it pure virtual, for now can be called only by invokeMethod()
    //       in order to simulate polymorphism
    void abortActivity();

    /**
     * Dump resource internals, for debugging.
     * @since 4.9
     */
    // KDE5: Make it pure virtual, for now can be called only by invokeMethod()
    //       in order to simulate polymorphism
    QString dumpResourceToString() const
    {
        return QString();
    }

protected:
    /**
     * Creates a base resource.
     *
     * @param id The instance id of the resource.
     */
    ResourceBase(const QString &id);

    /**
     * Destroys the base resource.
     */
    ~ResourceBase();

    /**
     * Call this method from retrieveItem() once the result is available.
     *
     * @param item The retrieved item.
     */
    void itemRetrieved(const Item &item);

    /**
     * Call this method from retrieveCollectionAttributes() once the result is available.
     *
     * @param collection The collection whose attributes got retrieved.
     * @since 4.6
     */
    void collectionAttributesRetrieved(const Collection &collection);

    /**
     * Resets the dirty flag of the given item and updates the remote id.
     *
     * Call whenever you have successfully written changes back to the server.
     * This implicitly calls changeProcessed().
     * @param item The changed item.
     */
    void changeCommitted(const Item &item);

    /**
     * Resets the dirty flag of all given items and updates remote ids.
     *
     * Call whenever you have successfully written changes back to the server.
     * This implicitly calls changeProcessed().
     * @param items Changed items
     *
     * @since 4.11
     */
    void changesCommitted(const Item::List &items);

    /**
     * Resets the dirty flag of the given tag and updates the remote id.
     *
     * Call whenever you have successfully written changes back to the server.
     * This implicitly calls changeProcessed().
     * @param tag Changed tag.
     *
     * @since 4.13
     */
    void changeCommitted(const Tag &tag);

    /**
     * Call whenever you have successfully handled or ignored a collection
     * change notification.
     *
     * This will update the remote identifier of @p collection if necessary,
     * as well as any other collection attributes.
     * This implicitly calls changeProcessed().
     * @param collection The collection which changes have been handled.
    */
    void changeCommitted(const Collection &collection);

    /**
     * Call this to supply the full folder tree retrieved from the remote server.
     *
     * @param collections A list of collections.
     * @see collectionsRetrievedIncremental()
    */
    void collectionsRetrieved(const Collection::List &collections);

    void tagsRetrieved(const Tag::List &tags, const QHash<QString, Item::List> &tagMembers);

    /**
     * Call this to supply incrementally retrieved collections from the remote server.
     *
     * @param changedCollections Collections that have been added or changed.
     * @param removedCollections Collections that have been deleted.
     * @see collectionsRetrieved()
     */
    void collectionsRetrievedIncremental(const Collection::List &changedCollections,
                                         const Collection::List &removedCollections);

    /**
     * Enable collection streaming, that is collections don't have to be delivered at once
     * as result of a retrieveCollections() call but can be delivered by multiple calls
     * to collectionsRetrieved() or collectionsRetrievedIncremental(). When all collections
     * have been retrieved, call collectionsRetrievalDone().
     * @param enable @c true if collection streaming should be enabled, @c false by default
     */
    void setCollectionStreamingEnabled(bool enable);

    /**
     * Call this method to indicate you finished synchronizing the collection tree.
     *
     * This is not needed if you use the built in syncing without collection streaming
     * and call collectionsRetrieved() or collectionRetrievedIncremental() instead.
     * If collection streaming is enabled, call this method once all collections have been delivered
     * using collectionsRetrieved() or collectionsRetrievedIncremental().
     */
    void collectionsRetrievalDone();

    /**
     * Allows to keep locally changed collection parts during the collection sync.
     *
     * This is useful for resources to be able to provide default values during the collection
     * sync, while preserving eventual more up-to date values.
     *
     * Valid values are attribute types and "CONTENTMIMETYPE" for the collections content mimetypes.
     *
     * By default this is enabled for the EntityDisplayAttribute.
     *
     * @param parts A set parts for which local changes should be preserved.
     * @since 4.14
    */
    void setKeepLocalCollectionChanges(const QSet<QByteArray> &parts);

    /**
     * Call this method to supply the full collection listing from the remote server. Items not present in the list
     * will be dropped from the Akonadi database.
     *
     * If the remote server supports incremental listing, it's strongly
     * recommended to use itemsRetrievedIncremental() instead.
     * @param items A list of items.
     * @see itemsRetrievedIncremental().
     */
    void itemsRetrieved(const Item::List &items);

    /**
     * Call this method when you want to use the itemsRetrieved() method
     * in streaming mode and indicate the amount of items that will arrive
     * that way.
     *
     * @warning By default this will end the item sync automatically once
     * sufficient items were delivered. To disable this and only make use
     * of the progress reporting, use setDisableAutomaticItemDeliveryDone()
     *
     * @note Use setItemStreamingEnabled( true ) + itemsRetrieved[Incremental]()
     * + itemsRetrieved() and avoid the automatic delivery based on the total
     * number of items.
     *
     * @param amount number of items that will arrive in streaming mode
     */
    void setTotalItems(int amount);

    /**
     * Disables the automatic completion of the item sync,
     * based on the number of delivered items.
     *
     * This ensures that the item sync only finishes once itemsRetrieved()
     * is called, while still making it possible to use the automatic progress
     * reporting based on setTotalItems().
     *
     * @note This needs to be called once, before the item sync started.
     *
     * @see setTotalItems(int)
     * @since 4.14
     */
    void setDisableAutomaticItemDeliveryDone(bool disable);

    /**
     * Enable item streaming.
     * Item streaming is disabled by default.
     * @param enable @c true if items are delivered in chunks rather in one big block.
     */
    void setItemStreamingEnabled(bool enable);

    /**
     * Set transaction mode for item sync'ing.
     * @param mode item transaction mode
     * @see Akonadi::ItemSync::TransactionMode
     * @since 4.6
     */
    void setItemTransactionMode(ItemSync::TransactionMode mode);

    /**
     * Set the fetch scope applied for item synchronization.
     * By default, the one set on the changeRecorder() is used. However, it can make sense
     * to specify a specialized fetch scope for synchronization to improve performance.
     * The rule of thumb is to remove anything from this fetch scope that does not provide
     * additional information regarding whether and item has changed or not. This is primarily
     * relevant for backends not supporting incremental retrieval.
     * @param fetchScope The fetch scope to use by the internal Akonadi::ItemSync instance.
     * @see Akonadi::ItemSync
     * @since 4.6
     */
    void setItemSynchronizationFetchScope(const ItemFetchScope &fetchScope);

    /**
     * Call this method to supply incrementally retrieved items from the remote server.
     *
     * @param changedItems Items changed in the backend.
     * @param removedItems Items removed from the backend.
     */
    void itemsRetrievedIncremental(const Item::List &changedItems,
                                   const Item::List &removedItems);

    /**
     * Call this method to indicate you finished synchronizing the current collection.
     *
     * This is not needed if you use the built in syncing without item streaming
     * and call itemsRetrieved() or itemsRetrievedIncremental() instead.
     * If item streaming is enabled, call this method once all items have been delivered
     * using itemsRetrieved() or itemsRetrievedIncremental().
     * @see retrieveItems()
     */
    void itemsRetrievalDone();

    /**
     * Call this method to remove all items and collections of the resource from the
     * server cache.
     *
     * The method should not be used anymore
     *
     * @see invalidateCache()
     * @since 4.3
     */
    void clearCache();

    /**
     * Call this method to invalidate all cached content in @p collection.
     *
     * The method should be used when the backend indicated that the cached content
     * is no longer valid.
     *
     * @param collection parent of the content to be invalidated in cache
     * @since 4.8
     */
    void invalidateCache(const Collection &collection);

    /**
     * Returns the collection that is currently synchronized.
     * @note Calling this method is only allowed during a collection synchronization task, that
     * is directly or indirectly from retrieveItems().
     */
    Collection currentCollection() const;

    /**
     * Returns the item that is currently retrieved.
     * @note Calling this method is only allowed during fetching a single item, that
     * is directly or indirectly from retrieveItem().
     */
    Item currentItem() const;

    /**
     * This method is called whenever the resource should start synchronize all data.
     */
    void synchronize();

    /**
     * This method is called whenever the collection with the given @p id
     * shall be synchronized.
     */
    void synchronizeCollection(qint64 id);

    /**
     * This method is called whenever the collection with the given @p id
     * shall be synchronized.
     * @param recursive if true, a recursive synchronization is done
     */
    void synchronizeCollection(qint64 id, bool recursive);

    /**
     * This method is called whenever the collection with the given @p id
     * shall have its attributes synchronized.
     *
     * @param id The id of the collection to synchronize
     * @since 4.6
     */
    void synchronizeCollectionAttributes(qint64 id);

    /**
     * Synchronizes the collection attributes.
     *
     * @param col The id of the collection to synchronize
     * @since 4.15
     */
    void synchronizeCollectionAttributes(const Akonadi::Collection &col);

    /**
     * Refetches the Collections.
     */
    void synchronizeCollectionTree();

    /**
     * Refetches Tags.
     */
    void synchronizeTags();

    /**
     * Stops the execution of the current task and continues with the next one.
     */
    void cancelTask();

    /**
     * Stops the execution of the current task and continues with the next one.
     * Additionally an error message is emitted.
     * @param error additional error message to be emitted
     */
    void cancelTask(const QString &error);

    /**
     * Stops the execution of the current task and continues with the next one.
     * The current task will be tried again later.
     *
     * This can be used to delay the task processing until the resource has reached a safe
     * state, e.g. login to a server succeeded.
     *
     * @note This does not change the order of tasks so if there is no task with higher priority
     *       e.g. a custom task added with #Prepend the deferred task will be processed again.
     *
     * @since 4.3
     */
    void deferTask();

    /**
     * Inherited from AgentBase.
     */
    void doSetOnline(bool online);

    /**
     * Indicate the use of hierarchical remote identifiers.
     *
     * This means that it is possible to have two different items with the same
     * remoteId in different Collections.
     *
     * This should be called in the resource constructor as needed.
     *
     * @param enable whether to enable use of hierarchical remote identifiers
     * @since 4.4
     */
    void setHierarchicalRemoteIdentifiersEnabled(bool enable);

    friend class ResourceScheduler;
    friend class ::ResourceState;

    /**
     * Describes the scheduling priority of a task that has been queued
     * for execution.
     *
     * @see scheduleCustomTask
     * @since 4.4
     */
    enum SchedulePriority {
        Prepend,            ///< The task will be executed as soon as the current task has finished.
        AfterChangeReplay,  ///< The task is scheduled after the last ChangeReplay task in the queue
        Append              ///< The task will be executed after all tasks currently in the queue are finished
    };

    /**
     * Schedules a custom task in the internal scheduler. It will be queued with
     * all other tasks such as change replays and retrieval requests and eventually
     * executed by calling the specified method. With the priority parameter the
     * time of execution of the Task can be influenced. @see SchedulePriority
     * @param receiver The object the slot should be called on.
     * @param method The name of the method (and only the name, not signature, not SLOT(...) macro),
     * that should be called to execute this task. The method has to be a slot and take a QVariant as
     * argument.
     * @param argument A QVariant argument passed to the method specified above. Use this to pass task
     * parameters.
     * @param priority Priority of the task. Use this to influence the position in
     * the execution queue.
     * @since 4.4
     */
    void scheduleCustomTask(QObject *receiver, const char *method, const QVariant &argument, SchedulePriority priority = Append);

    /**
     * Indicate that the current task is finished. Use this method from the slot called via scheduleCustomTaks().
     * As with all the other callbacks, make sure to either call taskDone() or cancelTask()/deferTask() on all
     * exit paths, otherwise the resource will hang.
     * @since 4.4
     */
    void taskDone();

    /**
     * Dump the contents of the current ChangeReplay
     * @since 4.8.1
     */
    QString dumpNotificationListToString() const;

    /**
     *  Dumps memory usage information to stdout.
     *  For now it outputs the result of glibc's mallinfo().
     *  This is useful to check if your memory problems are due to poor management by glibc.
     *  Look for a high value on fsmblks when interpreting results.
     *  man mallinfo for more details.
     *  @since 4.11
     */
    void dumpMemoryInfo() const;

    /**
     *  Returns a string with memory usage information.
     *  @see dumpMemoryInfo()
     *
     *  @since 4.11
     */
    QString dumpMemoryInfoToString() const;

    /**
     * Dump the state of the scheduler
     * @since 4.8.1
     */
    QString dumpSchedulerToString() const;

private:
    static QString parseArguments(int, char **);
    static int init(ResourceBase *r);

    // dbus resource interface
    friend class ::Akonadi__ResourceAdaptor;

    bool requestItemDelivery(qint64 uid, const QString &remoteId, const QString &mimeType, const QStringList &parts);

    QString requestItemDeliveryV2(qint64 uid, const QString &remoteId, const QString &mimeType, const QStringList &parts);

private:
    Q_DECLARE_PRIVATE(ResourceBase)

    Q_PRIVATE_SLOT(d_func(), void slotAbortRequested())
    Q_PRIVATE_SLOT(d_func(), void slotDeliveryDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotCollectionSyncDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotDeleteResourceCollection())
    Q_PRIVATE_SLOT(d_func(), void slotDeleteResourceCollectionDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotCollectionDeletionDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotInvalidateCache(const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void slotLocalListDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotSynchronizeCollection(const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void slotCollectionListDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotSynchronizeCollectionAttributes(const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void slotCollectionListForAttributesDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotCollectionAttributesSyncDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotItemSyncDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotPercent(KJob *, unsigned long))
    Q_PRIVATE_SLOT(d_func(), void slotDelayedEmitProgress())
    Q_PRIVATE_SLOT(d_func(), void slotPrepareItemRetrieval(const Akonadi::Item &item))
    Q_PRIVATE_SLOT(d_func(), void slotPrepareItemRetrievalResult(KJob *))
    Q_PRIVATE_SLOT(d_func(), void changeCommittedResult(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotSessionReconnected())
    Q_PRIVATE_SLOT(d_func(), void slotRecursiveMoveReplay(RecursiveMover *))
    Q_PRIVATE_SLOT(d_func(), void slotRecursiveMoveReplayResult(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotTagSyncDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotSynchronizeTags())
    Q_PRIVATE_SLOT(d_func(), void slotItemRetrievalCollectionFetchDone(KJob *));
    Q_PRIVATE_SLOT(d_func(), void slotAttributeRetrievalCollectionFetchDone(KJob *));
};

}

#ifndef AKONADI_RESOURCE_MAIN
/**
 * Convenience Macro for the most common main() function for Akonadi resources.
 */
#define AKONADI_RESOURCE_MAIN( resourceClass )                       \
  int main( int argc, char **argv )                                  \
  {                                                                  \
    return Akonadi::ResourceBase::init<resourceClass>( argc, argv ); \
  }
#endif

#endif
