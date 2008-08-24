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

#include <QtCore/QObject>

namespace Akonadi {

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
 * To enable this, see fetchCollection(), fetchItemMetaData(), fetchItemData().
 *
 * @todo: distinguish between monitoring collection properties and collection content.
 * @todo: special case for collection content counts changed
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
     * Sets whether the specified collection shall be monitored for changes.
     *
     * @param collection The collection to monitor.
     *                   If this collection is Collection::root(), all collections
     *                   in the Akonadi storage will be monitored.
     */
    void setCollectionMonitored( const Collection &collection, bool monitored = true );

    /**
     * Sets whether the specified item shall be monitored for changes.
     *
     * @param item The item to monitor.
     */
    void setItemMonitored( const Item &item, bool monitored = true );

    /**
     * Sets whether the specified resource shall be monitored for changes.
     *
     * @param resource The resource identifier.
     */
    void setResourceMonitored( const QByteArray &resource, bool monitored = true );

    /**
     * Sets whether objects of the specified mime type shall be monitored for changes.
     *
     * @param mimetype The mime type to monitor.
     */
    void setMimeTypeMonitored( const QString &mimetype, bool monitored = true );

    /**
     * Sets whether all items shall be monitored.
     */
    void setAllMonitored( bool monitored = true );

    /**
     * Ignores all change notifications caused by the given session.
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
     */
    void itemLinked( const Akonadi::Item &item, const Akonadi::Collection &collection );

    /**
     * This signal is emitted if a reference to an item is removed from a virtual collection.
     * @param item The unlinked item.
     * @param collection The collection the item is unlinked from.
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
     * This signal is emitted if a monitored collection has been changed (properties or content)
     * or has been reparented.
     *
     * @param collection The changed collection.
     */
    void collectionChanged( const Akonadi::Collection &collection );

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

  protected:
    //@cond PRIVATE
    MonitorPrivate *d_ptr;
    explicit Monitor( MonitorPrivate *d, QObject *parent = 0 );
    //@endcond

  private:
    Q_DECLARE_PRIVATE( Monitor )

    //@cond PRIVATE
    Q_PRIVATE_SLOT( d_ptr, void slotStatisticsChangedFinished( KJob* ) )
    Q_PRIVATE_SLOT( d_ptr, void slotFlushRecentlyChangedCollections() )
    Q_PRIVATE_SLOT( d_ptr, void slotNotify( const Akonadi::NotificationMessage::List& ) )
    Q_PRIVATE_SLOT( d_ptr, void slotItemJobFinished( KJob* ) )
    Q_PRIVATE_SLOT( d_ptr, void slotCollectionJobFinished( KJob* ) )
    //@endcond
};

}

#endif
