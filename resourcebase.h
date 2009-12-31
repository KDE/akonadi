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

class KJob;
class ResourceAdaptor;

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
 * Note that these three functions don't get the full payload of the items by default,
 * you need to change the item fetch scope of the change recorder to fetch the full
 * payload. This can be expensive with big payloads, though.<br>
 * Once you have handled changes in these methods call changeCommitted().
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
     */
    template <typename T>
    static int init( int argc, char **argv )
    {
      const QString id = parseArguments( argc, argv );
      KApplication app;
      T* r = new T( id );

      // check if T also inherits AgentBase::Observer and
      // if it does, automatically register it on itself
      Observer *observer = dynamic_cast<Observer*>( r );
      if ( observer != 0 )
        r->registerObserver( observer );

      return init( r );
    }

    /**
     * This method is used to set the name of the resource.
     */
    //FIXME_API: make sure location is renamed to this by resourcebase
    void setName( const QString &name );

    /**
     * Returns the name of the resource.
     */
    QString name() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the name of the resource has changed.
     *
     * @param name The new name of the resource.
     */
    void nameChanged( const QString &name );

    /**
     * Emitted when a full synchronization has been completed.
     */
    void synchronized();

  protected Q_SLOTS:
    /**
     * Retrieve the collection tree from the remote server and supply it via
     * collectionsRetrieved() or collectionsRetrievedIncremental().
     * @see collectionsRetrieved(), collectionsRetrievedIncremental()
     */
    virtual void retrieveCollections() = 0;

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
     * @see itemsRetrieved( const Item::List& ), itemsRetrievedIncremental(), itemsRetrieved(), currentCollection()
     */
    virtual void retrieveItems( const Akonadi::Collection &collection ) = 0;

    /**
     * Retrieve a single item from the backend. The item to retrieve is provided as @p item.
     * Add the requested payload parts and call itemRetrieved() when done.
     * @param item The empty item whose payload should be retrieved. Use this object when delivering
     * the result instead of creating a new item to ensure conflict detection will work.
     * @param parts The item parts that should be retrieved.
     * @return false if there is an immediate error when retrieving the item.
     * @see itemRetrieved()
     */
    virtual bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts ) = 0;

  protected:
    /**
     * Creates a base resource.
     *
     * @param id The instance id of the resource.
     */
    ResourceBase( const QString & id );

    /**
     * Destroys the base resource.
     */
    ~ResourceBase();

    /**
     * Call this method from retrieveItem() once the result is available.
     *
     * @param item The retrieved item.
     */
    void itemRetrieved( const Item &item );

    /**
     * Resets the dirty flag of the given item and updates the remote id.
     *
     * Call whenever you have successfully written changes back to the server.
     * This implicitly calls changeProcessed().
     * @param item The changed item.
     */
    void changeCommitted( const Item &item );

    /**
     * Call whenever you have successfully handled or ignored a collection
     * change notification.
     *
     * This will update the remote identifier of @p collection if necessary,
     * as well as any other collection attributes.
     * This implicitly calls changeProcessed().
     * @param collection The collection which changes have been handled.
    */
    void changeCommitted( const Collection &collection );

    /**
     * Call this to supply the full folder tree retrieved from the remote server.
     *
     * @param collections A list of collections.
     * @see collectionsRetrievedIncremental()
    */
    void collectionsRetrieved( const Collection::List &collections );

    /**
     * Call this to supply incrementally retrieved collections from the remote server.
     *
     * @param changedCollections Collections that have been added or changed.
     * @param removedCollections Collections that have been deleted.
     * @see collectionsRetrieved()
     */
    void collectionsRetrievedIncremental( const Collection::List &changedCollections,
                                          const Collection::List &removedCollections );

    /**
     * Enable collection streaming, that is collections don't have to be delivered at once
     * as result of a retrieveCollections() call but can be delivered by multiple calls
     * to collectionsRetrieved() or collectionsRetrievedIncremental(). When all collections
     * have been retrieved, call collectionsRetrievalDone().
     * @param enable @c true if collection streaming should be enabled, @c false by default
     */
    void setCollectionStreamingEnabled( bool enable );

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
     * Call this method to supply the full collection listing from the remote server.
     *
     * If the remote server supports incremental listing, it's strongly
     * recommended to use itemsRetrievedIncremental() instead.
     * @param items A list of items.
     * @see itemsRetrievedIncremental().
     */
    void itemsRetrieved( const Item::List &items );

    /**
     * Call this method when you want to use the itemsRetrieved() method
     * in streaming mode and indicate the amount of items that will arrive
     * that way.
     * @deprecated Use setItemStreamingEnabled( true ) + itemsRetrieved[Incremental]()
     * + itemsRetrieved() instead.
     */
    void setTotalItems( int amount );

    /**
     * Enable item streaming.
     * Item streaming is disabled by default.
     * @param enable @c true if items are delivered in chunks rather in one big block.
     */
    void setItemStreamingEnabled( bool enable );

    /**
     * Call this method to supply incrementally retrieved items from the remote server.
     *
     * @param changedItems Items changed in the backend.
     * @param removedItems Items removed from the backend.
     */
    void itemsRetrievedIncremental( const Item::List &changedItems,
                                    const Item::List &removedItems );

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
     * The method should be used whenever the configuration of the resource has changed
     * and therefor the cached items might not be valid any longer.
     *
     * @since 4.3
     */
    void clearCache();

    /**
     * Returns the collection that is currently synchronized.
     */
    Collection currentCollection() const;

    /**
     * Returns the item that is currently retrieved.
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
    void synchronizeCollection( qint64 id );

    /**
     * Refetches the Collections.
     */
    void synchronizeCollectionTree();

    /**
     * Stops the execution of the current task and continues with the next one.
     */
    void cancelTask();

    /**
     * Stops the execution of the current task and continues with the next one.
     * Additionally an error message is emitted.
     */
    void cancelTask( const QString &error );

    /**
     * Stops the execution of the current task and continues with the next one.
     * The current task will be tried again later.
     *
     * @since 4.3
     */
    void deferTask();

    /**
     * Inherited from AgentBase.
     */
    void doSetOnline( bool online );

    /**
     * Indicate the use of hierarchical remote identifiers.
     *
     * This means that it is possible to have two different items with the same
     * remoteId in different Collections.
     *
     * This should be called in the resource constructor as needed.
     *
     * @since 4.4
     */
    void setHierarchicalRemoteIdentifiersEnabled( bool enable );

    friend class ResourceScheduler;

    /**
     * Describes the scheduling priority of a task that has been queued
     * for execution.
     *
     * @see scheduleCustomTask
     * @since 4.4
     */
    enum SchedulePriority {
      Prepend,            ///> The task will be executed as soon as the current task has finished.
      AfterChangeReplay,  ///> The task is scheduled after the last ChangeReplay task in the queue
      Append              ///> The task will be executed after all tasks currently in the queue are finished
    };

    /**
     * Schedules a custom task in the internal scheduler. It will be queued with
     * all other tasks such as change replays and retrieval requests and eventually
     * executed by calling the specified method. With the priority parameter the
     * time of execution of the Task can be influenced. @see SchedulePriority
     * @param receiver The object the slot should be called on.
     * @param methodName The name of the method (and only the name, not signature, not SLOT(...) macro),
     * that should be called to execute this task. The method has to be a slot and take a QVariant as
     * argument.
     * @param argument A QVariant argument passed to the method specified above. Use this to pass task
     * parameters.
     * @param priority Priority of the task. Use this to influence the position in
     * the execution queue.
     * @since 4.4
     */
    void scheduleCustomTask( QObject* receiver, const char* method, const QVariant &argument, SchedulePriority priority = Append );

    /**
     * Indicate that the current task is finished. Use this method from the slot called via scheduleCustomTaks().
     * As with all the other callbacks, make sure to either call taskDone() or cancelTask()/deferTask() on all
     * exit paths, otherwise the resource will hang.
     * @since 4.4
     */
    void taskDone();

  private:
    static QString parseArguments( int, char** );
    static int init( ResourceBase *r );

    // dbus resource interface
    friend class ::ResourceAdaptor;

    bool requestItemDelivery( qint64 uid, const QString &remoteId, const QString &mimeType, const QStringList &parts );

  private:
    Q_DECLARE_PRIVATE( ResourceBase )

    Q_PRIVATE_SLOT( d_func(), void slotDeliveryDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotCollectionSyncDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotDeleteResourceCollection() )
    Q_PRIVATE_SLOT( d_func(), void slotDeleteResourceCollectionDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotCollectionDeletionDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotLocalListDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotSynchronizeCollection( const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void slotCollectionListDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotItemSyncDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotPercent( KJob*, unsigned long ) )
    Q_PRIVATE_SLOT( d_func(), void slotPrepareItemRetrieval( const Akonadi::Item& item ) )
    Q_PRIVATE_SLOT( d_func(), void slotPrepareItemRetrievalResult( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void changeCommittedResult( KJob* ) )
};

}

#ifndef AKONADI_RESOURCE_MAIN
/**
 * Convenience Macro for the most common main() function for Akonadi resources.
 */
#define AKONADI_RESOURCE_MAIN( resourceClass )                       \
  int main( int argc, char **argv )                            \
  {                                                            \
    return Akonadi::ResourceBase::init<resourceClass>( argc, argv ); \
  }
#endif

#endif
