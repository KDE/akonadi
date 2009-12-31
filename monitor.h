/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_MONITOR_H
#define AKONADI_MONITOR_H

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QtCore/QObject>

namespace Akonadi {

class CollectionFetchScope;
class CollectionStatistics;
class Item;
class ItemFetchScope;
class MonitorPrivate;
class Session;

/**
 * @short Monitors an item or collection for changes.
 *
 * The Monitor emits signals if some of these objects are changed or
 * removed or new ones are added to the Akonadi storage.
 *
 * Optionally, the changed objects can be fetched automatically from the server.
 * To enable this, see itemFetchScope() and collectionFetchScope().
 *
 * @todo Distinguish between monitoring collection properties and collection content.
 * @todo Special case for collection content counts changed
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT Monitor : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new monitor.
     *
     * @param parent The parent object.
     */
    explicit Monitor( QObject *parent = 0 );

    /**
     * Destroys the monitor.
     */
    virtual ~Monitor();

    /**
     * Sets whether the specified collection shall be monitored for changes. If
     * monitoring is turned on for the collection, all notifications for items
     * in that collection will be emitted, and its child collections will also
     * be monitored. Note that move notifications will be emitted if either one
     * of the collections involved is being monitored.
     *
     * Note that if a session is being ignored, this takes precedence over
     * setCollectionMonitored() on that session.
     *
     * @param collection The collection to monitor.
     *                   If this collection is Collection::root(), all collections
     *                   in the Akonadi storage will be monitored.
     * @param monitored Whether to monitor the collection.
     */
    void setCollectionMonitored( const Collection &collection, bool monitored = true );

    /**
     * Sets whether the specified item shall be monitored for changes.
     *
     * Note that if a session is being ignored, this takes precedence over
     * setItemMonitored() on that session.
     *
     * @param item The item to monitor.
     * @param monitored Whether to monitor the item.
     */
    void setItemMonitored( const Item &item, bool monitored = true );

    /**
     * Sets whether the specified resource shall be monitored for changes. If
     * monitoring is turned on for the resource, all notifications for
     * collections and items in that resource will be emitted.
     *
     * Note that if a session is being ignored, this takes precedence over
     * setResourceMonitored() on that session.
     *
     * @param resource The resource identifier.
     * @param monitored Whether to monitor the resource.
     */
    void setResourceMonitored( const QByteArray &resource, bool monitored = true );

    /**
     * Sets whether items of the specified mime type shall be monitored for changes.
     * If monitoring is turned on for the mime type, all notifications for items
     * matching that mime type will be emitted, but notifications for collections
     * matching that mime type will only be emitted if this is otherwise specified,
     * for example by setCollectionMonitored().
     *
     * Note that if a session is being ignored, this takes precedence over
     * setMimeTypeMonitored() on that session.
     *
     * @param mimetype The mime type to monitor.
     * @param monitored Whether to monitor the mime type.
     */
    void setMimeTypeMonitored( const QString &mimetype, bool monitored = true );

    /**
     * Sets whether all items shall be monitored.
     *
     * Note that if a session is being ignored, this takes precedence over
     * setAllMonitored() on that session.
     */
    void setAllMonitored( bool monitored = true );

    /**
     * Ignores all change notifications caused by the given session. This
     * overrides all other settings on this session.
     *
     * @param session The session you want to ignore.
     */
    void ignoreSession( Session *session );

    /**
     * Enables automatic fetching of changed collections from the Akonadi storage.
     *
     * @param enable @c true enables automatic fetching, @c false disables automatic fetching.
     */
    void fetchCollection( bool enable );

    /**
     * Enables automatic fetching of changed collection statistics information from
     * the Akonadi storage.
     *
     * @param enable @c true to enables automatic fetching, @c false disables automatic fetching.
     */
    void fetchCollectionStatistics( bool enable );

    /**
     * Sets the item fetch scope.
     *
     * Controls how much of an item's data is fetched from the server, e.g.
     * whether to fetch the full item payload or only meta data.
     *
     * @param fetchScope The new scope for item fetch operations.
     *
     * @see itemFetchScope()
     */
    void setItemFetchScope( const ItemFetchScope &fetchScope );

    /**
     * Returns the item fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the ItemFetchScope documentation
     * for an example.
     *
     * @return a reference to the current item fetch scope
     *
     * @see setItemFetchScope() for replacing the current item fetch scope
     */
    ItemFetchScope &itemFetchScope();

    /**
     * Sets the collection fetch scope.
     *
     * Controls which collections are monitored and how much of a collection's data
     * is fetched from the server.
     *
     * @param fetchScope The new scope for collection fetch operations.
     *
     * @see collectionFetchScope()
     * @since 4.4
     */
    void setCollectionFetchScope( const CollectionFetchScope &fetchScope );

    /**
     * Returns the collection fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the CollectionFetchScope documentation
     * for an example.
     *
     * @return a reference to the current collection fetch scope
     *
     * @see setCollectionFetchScope() for replacing the current collection fetch scope
     * @since 4.4
     */
    CollectionFetchScope &collectionFetchScope();

    /**
     * Returns the list of collections being monitored.
     *
     * @since 4.3
     */
    Collection::List collectionsMonitored() const;

    /**
     * Returns the set of items being monitored.
     *
     * @since 4.3
     */
    QList<Item::Id> itemsMonitored() const;

    /**
     * Returns the set of mimetypes being monitored.
     *
     * @since 4.3
     */
    QStringList mimeTypesMonitored() const;

    /**
     * Returns the set of identifiers for resources being monitored.
     *
     * @since 4.3
     */
    QList<QByteArray> resourcesMonitored() const;

    /**
     * Returns true if everything is being monitored.
     *
     * @since 4.3
     */
    bool isAllMonitored() const;

    /**
     * Sets the session used by the Monitor to communicate with the %Akonadi server.
     * If not set, the Akonadi::Session::defaultSession is used.
     *
     * @since 4.4
     */
    void setSession( Akonadi::Session *session );

    /**
     * Returns the Session used by the monitor to communicate with Akonadi.
     *
     * @since 4.4
     */
    Session* session() const;

  Q_SIGNALS:
    /**
     * This signal is emitted if a monitored item has changed, e.g. item parts have been modified.
     *
     * @param item The changed item.
     * @param partIdentifiers The identifiers of the item parts that has been changed.
     */
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );

    /**
     * This signal is emitted if a monitored item has been moved between two collections
     *
     * @param item The moved item.
     * @param collectionSource The collection the item has been moved from.
     * @param collectionDestination The collection the item has been moved to.
     */
    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                                               const Akonadi::Collection &collectionDestination );

    /**
     * This signal is emitted if an item has been added to a monitored collection in the Akonadi storage.
     *
     * @param item The new item.
     * @param collection The collection the item has been added to.
     */
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

    /**
     * This signal is emitted if
     *   - a monitored item has been removed from the Akonadi storage
     * or
     *   - a item has been removed from a monitored collection.
     *
     * @param item The removed item.
     */
    void itemRemoved( const Akonadi::Item &item );

    /**
     * This signal is emitted if a reference to an item is added to a virtual collection.
     * @param item The linked item.
     * @param collection The collection the item is linked to.
     *
     * @since 4.2
     */
    void itemLinked( const Akonadi::Item &item, const Akonadi::Collection &collection );

    /**
     * This signal is emitted if a reference to an item is removed from a virtual collection.
     * @param item The unlinked item.
     * @param collection The collection the item is unlinked from.
     *
     * @since 4.2
     */
    void itemUnlinked( const Akonadi::Item &item, const Akonadi::Collection &collection );

    /**
     * This signal is emitted if a new collection has been added to a monitored collection in the Akonadi storage.
     *
     * @param collection The new collection.
     * @param parent The parent collection.
     */
    void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );

    /**
     * This signal is emitted if a monitored collection has been changed (properties or content).
     *
     * @param collection The changed collection.
     */
    void collectionChanged( const Akonadi::Collection &collection );

    /**
     * This signal is emitted if a monitored collection has been changed (properties or attributes).
     *
     * @param collection The changed collection.
     * @param attributeNames The names of the collection attributes that have been changed.
     *
     * @since 4.4
     */
    void collectionChanged( const Akonadi::Collection &collection, const QSet<QByteArray> &attributeNames );

    /**
     * This signals is emitted if a monitored collection has been moved.
     *
     * @param collection The moved collection.
     * @param source The previous parent collection.
     * @param distination The new parent collection.
     *
     * @since 4.4
     */
    void collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &destination );

    /**
     * This signal is emitted if a monitored collection has been removed from the Akonadi storage.
     *
     * @param collection The removed collection.
     */
    void collectionRemoved( const Akonadi::Collection &collection );

    /**
     * This signal is emitted if the statistics information of a monitored collection
     * has changed.
     *
     * @param id The collection identifier of the changed collection.
     * @param statistics The updated collection statistics, invalid if automatic
     *                   fetching of statistics changes is disabled.
     */
    void collectionStatisticsChanged( Akonadi::Collection::Id id,
                                      const Akonadi::CollectionStatistics &statistics );

    /**
     * This signal is emitted if the Monitor starts or stops monitoring @p collection explicitly.
     * @param collection The collection
     * @param monitored Whether the collection is now being monitored or not.
     *
     * @since 4.3
     */
    void collectionMonitored( const Akonadi::Collection &collection, bool monitored );

    /**
     * This signal is emitted if the Monitor starts or stops monitoring @p item explicitly.
     * @param item The item
     * @param monitored Whether the item is now being monitored or not.
     *
     * @since 4.3
     */
    void itemMonitored( const Akonadi::Item &item, bool monitored );

    /**
     * This signal is emitted if the Monitor starts or stops monitoring the resource with the identifier @p identifier explicitly.
     * @param identifier The identifier of the resource.
     * @param monitored Whether the resource is now being monitored or not.
     *
     * @since 4.3
     */
    void resourceMonitored( const QByteArray &identifier, bool monitored );

    /**
     * This signal is emitted if the Monitor starts or stops monitoring @p mimeType explicitly.
     * @param mimeType The mimeType.
     * @param monitored Whether the mimeType is now being monitored or not.
     *
     * @since 4.3
     */
    void mimeTypeMonitored( const QString &mimeType, bool monitored );

    /**
     * This signal is emitted if the Monitor starts or stops monitoring everything.
     * @param monitored Whether everything is now being monitored or not.
     *
     * @since 4.3
     */
    void allMonitored( bool monitored );

  protected:
    //@cond PRIVATE
    friend class EntityTreeModel;
    friend class EntityTreeModelPrivate;
    MonitorPrivate *d_ptr;
    explicit Monitor( MonitorPrivate *d, QObject *parent = 0 );
    //@endcond

  private:
    Q_DECLARE_PRIVATE( Monitor )

    //@cond PRIVATE
    Q_PRIVATE_SLOT( d_ptr, void slotSessionDestroyed( QObject* ) )
    Q_PRIVATE_SLOT( d_ptr, void slotStatisticsChangedFinished( KJob* ) )
    Q_PRIVATE_SLOT( d_ptr, void slotFlushRecentlyChangedCollections() )
    Q_PRIVATE_SLOT( d_ptr, void slotNotify( const Akonadi::NotificationMessage::List& ) )
    Q_PRIVATE_SLOT( d_ptr, void dataAvailable() )

    friend class ResourceBasePrivate;
    //@endcond
};

}

#endif
