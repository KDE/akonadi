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
#include "resource.h"

#include <libakonadi/collection.h>
#include <libakonadi/job.h>

#include <kapplication.h>

#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtDBus/QDBusContext>

class KJob;
class ResourceAdaptor;

namespace Akonadi {

class Item;
class Job;
class Session;

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
 * X-Akonadi-Capabilitites<supported-mimetype>
 * X-Akonadi-Identifier=akonadi_my_resource
 *   \endcode
 * @todo what is capabilities used for?
 *
 * <h5>Handling PIM Items</h5>
 *
 * To follow item changes in the backend, the following steps are necessary:
 * - Implement synchronizeCollection() to synchronize all items in the given
 *   collection. You have to do item synchronization manually here, there are
 *   no convenience methods provided as for collections (yet).
 * - Call collectionSynchronized() when done.
 * @todo Convenience methods for item synchronization
 *
 * To fetch item data on demand, the method requestItemDelivery() needs to be
 * reimplemented. Fetch the requested data there, create a ItemStoreJob to store
 * the data and call deliverItem().
 *
 * To write local changes back to the backend, you need to re-implement
 * the following three methods:
 * - itemAdded()
 * - itemChanged()
 * - itemRemoved()
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
 * These methods are called whenever a local collection related to this resource is
 * added, modified or deleted. They are only called if the resource is online, otherwise
 * all changes are recorded and replayed as soon the resource is online again.
 *
 * @todo Convenience base class for collection-less resources
 */
class AKONADI_EXPORT ResourceBase : public Resource, protected QDBusContext
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
     * This method is called whenever the resource shall show its configuration dialog
     * to the user. It will be automatically called when the resource is started for
     * the first time.
     */
    virtual void configure();

    /**
     * This method is called whenever the resource should start synchronization.
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
     * Returns the instance identifier of this resource.
     */
    QString identifier() const;

    /**
     * This method is called when the resource is removed from
     * the system, so it can do some cleanup stuff.
     */
    virtual void cleanup() const;

    /**
     * This method is called from the crash handler, don't call
     * it manually.
     */
    void crashHandler( int signal );

    virtual bool isOnline() const;
    virtual void setOnline( bool state );

  public Q_SLOTS:
    /**
     * This method is called to quit the resource.
     *
     * Before the application is terminated @see aboutToQuit() is called,
     * which can be reimplemented to do some session cleanup (e.g. disconnecting
     * from groupware server).
     */
    void quit();

    /**
      Enables change recording. When change recording is enabled all changes are
      stored internally and replayed as soon as change recording is disabled.
      @param enable True to enable change recording, false to disable change recording.
    */
    void enableChangeRecording( bool enable );

  protected Q_SLOTS:
    /**
      Retrieve the collection tree from the remote server and supply it via
      collectionsRetrieved().
    */
    virtual void retrieveCollections() = 0;

    /**
      Synchronize the given collection with the backend.
      Call collectionSynchronized() once you are finished.
      @param collection The collection to sync.
      @see collectionSynchronized(), currentCollection()
    */
    virtual void synchronizeCollection( const Collection &collection ) = 0;

    /**
     * This method is called whenever an external query for putting data in the
     * storage is received. Must be reimplemented in any resource.
     *
     * @param ref The DataReference of this item.
     * @param parts The item parts that should be fetched.
     * @param msg QDBusMessage to pass along for delayed reply.
     */
    virtual bool requestItemDelivery( const DataReference &ref, const QStringList &parts, const QDBusMessage &msg ) = 0;

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
     * This method shall be used to report warnings.
     */
    void warning( const QString& message );

    /**
     * This method shall be used to report errors.
     */
    void error( const QString& message );

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
     * This method is called whenever the application is about to
     * quit.
     *
     * Reimplement this method to do session cleanup (e.g. disconnecting
     * from groupware server).
     */
    virtual void aboutToQuit();

    /**
     * This method returns the settings object which has to be used by the
     * resource to store its configuration data.
     *
     * Don't delete this object!
     */
    QSettings* settings();

    /**
     * Returns a session for communicating with the storage backend. It should
     * be used for all jobs.
     */
    Session* session();

    /**
      Call this method from in requestItemDelivery(). It will generate an appropriate
      D-Bus reply as soon as the given job has finished.
      @param job The job which actually delivers the item.
      @param msg The D-Bus message requesting the delivery.
    */
    bool deliverItem( Akonadi::Job* job, const QDBusMessage &msg );

    /**
      Reimplement to handle adding of new items.
      @param item The newly added item.
    */
    virtual void itemAdded( const Item &item, const Collection &collection );

    /**
      Reimplement to handle changes to existing items.
      @param item The changed item.
      @param partIdentifiers The identifiers of the item parts that has been changed.
    */
    virtual void itemChanged( const Item &item, const QStringList &partIdentifiers  );

    /**
      Reimplement to handle deletion of items.
      @param ref DataReference to the deleted item.
    */
    virtual void itemRemoved( const DataReference &ref );

    /**
      Reimplement to handle adding of new collections.
      @param collection The newly added collection.
      @param parent The parent collection.
    */
    virtual void collectionAdded( const Collection &collection, const Collection &parent );

    /**
      Reimplement to handle changes to existing collections.
      @param collection The changed collection.
    */
    virtual void collectionChanged( const Collection &collection );

    /**
      Reimplement to handle deletion of collections.
      @param id The id of the deleted collection.
      @param remoteId The remote id of the deleted collection.
    */
    virtual void collectionRemoved( int id, const QString &remoteId );

    /**
      Resets the dirty flag of the given item and updates the remote id.
      Call whenever you have successfully written changes back to the server.
      @param ref DataReference of the item.
    */
    void changesCommitted( const DataReference &ref );

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
      Call this method to indicate you finished synchronizing the current
      collection.
      @see synchronizeCollection()
    */
    void collectionSynchronized();

    /**
      Returns the collection that is currently synchronized.
    */
    Collection currentCollection() const;

  private:
    static QString parseArguments( int, char** );
    static int init( ResourceBase *r );

    // dbus resource interface
    friend class ::ResourceAdaptor;
    void synchronizeCollection( int collectionId );
    bool requestItemDelivery( int uid, const QString &remoteId, const QStringList &parts );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void slotDeliveryDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void slotItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d, void slotItemChanged( const Akonadi::Item&, const QStringList& ) )
    Q_PRIVATE_SLOT( d, void slotItemRemoved( const Akonadi::DataReference& ) )
    Q_PRIVATE_SLOT( d, void slotCollectionAdded( const Akonadi::Collection&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d, void slotCollectionChanged( const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d, void slotCollectionRemoved( int, const QString& ) )
    Q_PRIVATE_SLOT( d, void slotReplayNextItem() )
    Q_PRIVATE_SLOT( d, void slotReplayItemAdded( KJob* ) )
    Q_PRIVATE_SLOT( d, void slotReplayItemChanged( KJob* ) )
    Q_PRIVATE_SLOT( d, void slotReplayCollectionAdded( KJob* ) )
    Q_PRIVATE_SLOT( d, void slotReplayCollectionChanged( KJob* ) )
    Q_PRIVATE_SLOT( d, void slotCollectionSyncDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void slotLocalListDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void slotSynchronizeCollection(const Collection &col) )
    Q_PRIVATE_SLOT( d, void slotCollectionListDone( KJob* ) )
};

}

#endif
