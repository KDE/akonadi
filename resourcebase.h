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

#include "libakonadi_export.h"
#include <libakonadi/agentbase.h>

#include <libakonadi/collection.h>
#include <libakonadi/item.h>
#include <libakonadi/job.h>

#include <kapplication.h>

class KJob;
class ResourceAdaptor;

namespace Akonadi {

class Item;
class Job;
class Session;
class ResourceBasePrivate;

/**
 * This class should be used as a base class by all resource agents,
 * since it encapsulates large parts of the protocol between
 * resource agent, agent manager and storage.
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
 * Once you have handled changes in these methods call changesCommitted().
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
 * Once you have handled changes in these methods call changesCommitted().
 * These methods are called whenever a local collection related to this resource is
 * added, modified or deleted. They are only called if the resource is online, otherwise
 * all changes are recorded and replayed as soon the resource is online again.
 *
 * @todo Convenience base class for collection-less resources
 */
class AKONADI_EXPORT ResourceBase : public AgentBase
{
  Q_OBJECT

  public:
    /**
     * This enum describes the different states the
     * resource can be in.
     */
    enum Status
    {
      Ready = 0,
      Syncing,
      Error
    };

    /**
     * Use this method in the main function of your resource
     * application to initialize your resource subclass.
     * This method also takes care of creating a KApplication
     * object and parsing command line arguments.
     *
     * \code
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
     * \endcode
     */
    template <typename T>
    static int init( int argc, char **argv )
    {
      QString id = parseArguments( argc, argv );
      KApplication app;
      T* r = new T( id );
      return init( r );
    }

    /**
     * This method returns the current status code of the resource.
     *
     * The following return values are possible:
     *
     *  0 - Ready
     *  1 - Syncing
     *  2 - Error
     */
    virtual int status() const;

    /**
     * This method returns an i18n'ed description of the current status code.
     */
    virtual QString statusMessage() const;

    /**
     * This method returns the current progress of the resource in percentage.
     */
    virtual uint progress() const;

    /**
     * This method returns an i18n'ed description of the current progress.
     */
    virtual QString progressMessage() const;

    /**
     * This method is called whenever the resource should start synchronize all data.
     */
    virtual void synchronize();

    /**
     * This method is used to set the name of the resource.
     */
    virtual void setName( const QString &name );

    /**
     * Returns the name of the resource.
     */
    virtual QString name() const;

    /**
     * This method is called from the crash handler, don't call
     * it manually.
     */
    void crashHandler( int signal );

    virtual bool isOnline() const;
    virtual void setOnline( bool state );

  public Q_SLOTS:
    /**
     * This method is called whenever the resource shall show its configuration dialog
     * to the user. It will be automatically called when the resource is started for
     * the first time.
     */
    virtual void configure();

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the status of the resource has changed.
     *
     * @param status The status id of the resource (@see Status).
     * @param message An i18n'ed message which describes the status in detail.
     */
    void statusChanged( int status, const QString &message );

    /**
     * This signal is emitted whenever the progress information of the resource
     * has changed.
     *
     * @param progress The progress in percent (0 - 100).
     * @param message An i18n'ed message which describes the progress in detail.
     */
    void progressChanged( uint progress, const QString &message );

    /**
     * This signal is emitted whenever the name of the resource has changed.
     *
     * @param name The new name of the resource.
     */
    void nameChanged( const QString &name );

  protected Q_SLOTS:
    /**
      Retrieve the collection tree from the remote server and supply it via
      collectionsRetrieved() or collectionsRetrievedIncremental().
      @see collectionsRetrieved(), collectionsRetrievedIncremental()
    */
    virtual void retrieveCollections() = 0;

    /**
      Retrieve all (new/changed) items in collection @p collection.
      It is recommended to use incremental retrieval if the backend supports that
      and provide the result by calling itemsRetrievedIncremental().
      If incremental retrieval is not possible, provide the full listing by calling
      itemsRetrieved( const Item::List& ).
      In any case, ensure that all items have a correctly set remote identifier
      to allow synchronizing with already locally existing items.
      In case you don't want to use the built-in item syncing code, store the retrived
      items manually and call itemsRetrieved() once you are done.
      @param collection The collection to sync.
      @param parts The items parts that should be retrieved.
      @see itemsRetrieved( const Item::List &), itemsRetrievedIncremental(), itemsRetrieved(), currentCollection()
    */
    virtual void retrieveItems( const Akonadi::Collection &collection, const QStringList &parts ) = 0;

    /**
      Retrieve a single item from the backend. The item to retrieve is provided as @p item.
      Add the requested payload parts and call itemRetrieved() when done.
      @param item The empty item which payload should be retrieved. Use this object when delivering
      the result instead of creating a new item to ensure conflict detection to work.
      @param parts The item parts that should be retrieved.
      @return false if there is an immediate error when retrieving the item.
      @see itemRetrieved()
    */
    virtual bool retrieveItem( const Akonadi::Item &item, const QStringList &parts ) = 0;

    // reimplemnted from AgentBase
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemChanged( const Akonadi::Item &item, const QStringList &partIdentifiers );
    void itemRemoved( const Akonadi::DataReference &ref );
    void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    void collectionChanged( const Akonadi::Collection &collection );
    void collectionRemoved( int id, const QString &remoteId );

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
     * This method shall be used to signal a state change.
     *
     * @param status The new status of the resource.
     * @param message An i18n'ed description of the status. If message
     *                is empty, the default description for the status is used.
     */
    void changeStatus( Status status, const QString &message = QString() );

    /**
     * This method shall be used to signal a progress change.
     *
     * @param progress The new progress of the resource in percentage.
     * @param message An i18n'ed description of the progress.
     */
    void changeProgress( uint progress, const QString &message = QString() );

    /**
      Call this method from retrieveItem() once the result is available.
      @param job The job which actually delivers the item.
      @param msg The D-Bus message requesting the delivery.
    */
    void itemRetrieved( const Item &item );

    /**
      Resets the dirty flag of the given item and updates the remote id.
      Call whenever you have successfully written changes back to the server.
      @param ref DataReference of the item.
    */
    void changesCommitted( const DataReference &ref );

    /**
      Call whenever you have successfully handled or ignored a collection
      change notification. This will update the remote identifier of
      @p collection if necessary, as well as any other collection attributes.
      @param collection The collection which changes have been handled.
    */
    void changesCommitted( const Collection &collection );

    /**
      Call this to supply the full folder tree retrieved from the remote server.
      @param collections A list of collections.
      @see collectionsRetrievedIncremental()
    */
    void collectionsRetrieved( const Collection::List &collections );

    /**
      Call this to supply incrementally retrieved collections from the remote
      server.
      @param changedCollections Collections that have been added or changed.
      @param removedCollections Collections that have been deleted.
      @see collectionsRetrieved()
    */
    void collectionsRetrievedIncremental( const Collection::List &changedCollections,
                                          const Collection::List &removedCollections );

    /**
      Call this methods to supply the full collection listing from the remote
      server. If the remote server supports incremental listing, it's strongly
      recommended to use itemsRetrievedIncremental() instead.
      @param items A list of items.
      @see itemsRetrievedIncremental().
    */
    void itemsRetrieved( const Item::List &items );

    /**
      Call this method to supply incrementally retrieved items from the remote
      server.
      @param changedItems Items changed in the backend
      @param removedItems Items removed from the backend
    */
    void itemsRetrievedIncremental( const Item::List &changedItems,
                                    const Item::List &removedItems );

    /**
      Call this method to indicate you finished synchronizing the current
      collection. This is not needed if you use the built in syncing
      and call itemsRetrieved() or itemsRetrievedIncremental() instead.
      @see retrieveItems()
    */
    void itemsRetrieved();

    /**
      Returns the collection that is currently synchronized.
    */
    Collection currentCollection() const;

    /**
      Returns the item that is currently retrieved.
    */
    Item currentItem() const;

  private:
    static QString parseArguments( int, char** );
    static int init( ResourceBase *r );

    // dbus resource interface
    friend class ::ResourceAdaptor;
    void synchronizeCollectionTree();
    void synchronizeCollection( int collectionId );
    bool requestItemDelivery( int uid, const QString &remoteId, const QStringList &parts );

  private:
    Q_DECLARE_PRIVATE( ResourceBase )

    Q_PRIVATE_SLOT( d_func(), void slotDeliveryDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotCollectionSyncDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotLocalListDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotSynchronizeCollection(const Akonadi::Collection &col, const QStringList &parts) )
    Q_PRIVATE_SLOT( d_func(), void slotCollectionListDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotItemSyncDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void slotPercent( KJob*, unsigned long ) )
};

}

#endif
