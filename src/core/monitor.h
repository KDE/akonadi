/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"
#include "tag.h"

#include <QObject>

#include <memory>

namespace Akonadi
{
class CollectionFetchScope;
class CollectionStatistics;
class Item;
class ItemFetchScope;
class MonitorPrivate;
class Session;
class TagFetchScope;
class NotificationSubscriber;
class ChangeNotification;

namespace Protocol
{
class Command;
}

/*!
 * \brief Monitors an item or collection for changes.
 *
 * The Monitor emits signals if some of these objects are changed or
 * removed or new ones are added to the Akonadi storage.
 *
 * There are various ways to filter these notifications. There are three types of filter
 * evaluation:
 * - (-) removal-only filter, ie. if the filter matches the notification is dropped,
 *   if not filter evaluation continues with the next one
 * - (+) pass-exit filter, ie. if the filter matches the notification is delivered,
 *   if not evaluation is continued
 * - (f) final filter, ie. evaluation ends here if the corresponding filter criteria is set,
 *   the notification is delivered depending on the result, evaluation is only continued
 *   if no filter criteria is defined
 *
 * The following filter are available, listed in evaluation order:
 * (1) ignored sessions (-)
 * (2) monitor everything (+)
 * (3a) resource and mimetype filters (f) (items only)
 * (3b) resource filters (f) (collections only)
 * (4) item is monitored (+)
 * (5) collection is monitored (+)
 *
 * Optionally, the changed objects can be fetched automatically from the server.
 * To enable this, see itemFetchScope() and collectionFetchScope().
 *
 * Note that as a consequence of rule 3a, it is not possible to monitor (more than zero resources
 * OR more than zero mimetypes) AND more than zero collections.
 *
 * \todo Distinguish between monitoring collection properties and collection content.
 * \todo Special case for collection content counts changed
 *
 * \class Akonadi::Monitor
 * \inheaders Akonadi/Monitor
 * \inmodule AkonadiCore
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT Monitor : public QObject
{
    Q_OBJECT

public:
    enum Type {
        /*!
         * \internal This must be kept in sync with Akonadi::NotificationMessageV2::Type
         */
        Collections = 1,
        Items,
        Tags,
        /*!
         * Listen to subscription changes of other Monitors connected to Akonadi.
         * This is only for debugging purposes and should not be used in real
         * applications.
         * \since 5.4
         */
        Subscribers,
        /*!
         * Listens to all notifications being emitted by the server and provides
         * additional information about them. This is only for debugging purposes
         * and should not be used in real applications.
         *
         * \
ote Enabling monitoring this type has performance impact on the
         * Akonadi Server.
         *
         * \since 5.4
         */
        Notifications
    };

    /*!
     * Creates a new monitor.
     *
     * \a parent The parent object.
     */
    explicit Monitor(QObject *parent = nullptr);

    /*!
     * Destroys the monitor.
     */
    ~Monitor() override;

    /*!
     * Sets whether the specified collection shall be monitored for changes. If
     * monitoring is turned on for the collection, all notifications for items
     * in that collection will be emitted, and its child collections will also
     * be monitored. Note that move notifications will be emitted if either one
     * of the collections involved is being monitored.
     *
     * Note that if a session is being ignored, this takes precedence over
     * setCollectionMonitored() on that session.
     *
     * \a collection The collection to monitor.
     *                   If this collection is Collection::root(), all collections
     *                   in the Akonadi storage will be monitored.
     * \a monitored Whether to monitor the collection.
     */
    void setCollectionMonitored(const Collection &collection, bool monitored = true);

    /*!
     * Sets whether the specified item shall be monitored for changes.
     *
     * Note that if a session is being ignored, this takes precedence over
     * setItemMonitored() on that session.
     *
     * \a item The item to monitor.
     * \a monitored Whether to monitor the item.
     */
    void setItemMonitored(const Item &item, bool monitored = true);

    /*!
     * Sets whether the specified resource shall be monitored for changes. If
     * monitoring is turned on for the resource, all notifications for
     * collections and items in that resource will be emitted.
     *
     * Note that if a session is being ignored, this takes precedence over
     * setResourceMonitored() on that session.
     *
     * \a resource The resource identifier.
     * \a monitored Whether to monitor the resource.
     */
    void setResourceMonitored(const QByteArray &resource, bool monitored = true);

    /*!
     * Sets whether items of the specified mime type shall be monitored for changes.
     * If monitoring is turned on for the mime type, all notifications for items
     * matching that mime type will be emitted, but notifications for collections
     * matching that mime type will only be emitted if this is otherwise specified,
     * for example by setCollectionMonitored().
     *
     * Note that if a session is being ignored, this takes precedence over
     * setMimeTypeMonitored() on that session.
     *
     * \a mimetype The mime type to monitor.
     * \a monitored Whether to monitor the mime type.
     */
    void setMimeTypeMonitored(const QString &mimetype, bool monitored = true);

    /*!
     * Sets whether the specified tag shall be monitored for changes.
     *
     * Same rules as for item monitoring apply.
     *
     * \a tag Tag to monitor.
     * \a monitored Whether to monitor the tag.
     * \since 4.13
     */
    void setTagMonitored(const Tag &tag, bool monitored = true);

    /*!
     * Sets whether given type (Collection, Item, Tag should be monitored).
     *
     * By default all types are monitored, but once you change one, you have
     * to explicitly enable all other types you want to monitor.
     *
     * \a type Type to monitor.
     * \a monitored Whether to monitor the type
     * \since 4.13
     */
    void setTypeMonitored(Type type, bool monitored = true);

    /*!
     * Sets whether all items shall be monitored.
     * \a monitored sets all items as monitored if set as \\ true
     * Note that if a session is being ignored, this takes precedence over
     * setAllMonitored() on that session.
     */
    void setAllMonitored(bool monitored = true);

    void setExclusive(bool exclusive = true);
    [[nodiscard]] bool exclusive() const;

    /*!
     * Ignores all change notifications caused by the given session. This
     * overrides all other settings on this session.
     *
     * \a session The session you want to ignore.
     */
    void ignoreSession(Session *session);

    /*!
     * Enables automatic fetching of changed collections from the Akonadi storage.
     *
     * \a enable \\ true enables automatic fetching, \\ false disables automatic fetching.
     */
    void fetchCollection(bool enable);

    /*!
     * Enables automatic fetching of changed collection statistics information from
     * the Akonadi storage.
     *
     * \a enable \\ true to enables automatic fetching, \\ false disables automatic fetching.
     */
    void fetchCollectionStatistics(bool enable);

    /*!
     * Sets the item fetch scope.
     *
     * Controls how much of an item's data is fetched from the server, e.g.
     * whether to fetch the full item payload or only meta data.
     *
     * \a fetchScope The new scope for item fetch operations.
     *
     * \sa itemFetchScope()
     */
    void setItemFetchScope(const ItemFetchScope &fetchScope);

    /*!
     * Instructs the monitor to fetch only those parts that were changed and
     * were requested in the fetch scope.
     *
     * This is taken in account only for item modifications.
     * Example usage:
     * \code
     *   monitor->itemFetchScope().fetchFullPayload( true );
     *   monitor->fetchChangedOnly(true);
     * \endcode
     *
     * In the example if an item was changed, but its payload was not, the full
     * payload will not be retrieved.
     * If the item's payload was changed, the monitor retrieves the changed
     * payload as well.
     *
     * The default is to fetch everything requested.
     *
     * \since 4.8
     *
     * \a enable \\ true to enable the feature, \\ false means everything
     * that was requested will be fetched.
     */
    void fetchChangedOnly(bool enable);

    /*!
     * Returns the item fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the ItemFetchScope documentation
     * for an example.
     *
     * Returns a reference to the current item fetch scope
     *
     * \sa setItemFetchScope() for replacing the current item fetch scope
     */
    ItemFetchScope &itemFetchScope();

    /*!
     * Sets the collection fetch scope.
     *
     * Controls which collections are monitored and how much of a collection's data
     * is fetched from the server.
     *
     * \a fetchScope The new scope for collection fetch operations.
     *
     * \sa collectionFetchScope()
     * \since 4.4
     */
    void setCollectionFetchScope(const CollectionFetchScope &fetchScope);

    /*!
     * Returns the collection fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the CollectionFetchScope documentation
     * for an example.
     *
     * Returns a reference to the current collection fetch scope
     *
     * \sa setCollectionFetchScope() for replacing the current collection fetch scope
     * \since 4.4
     */
    CollectionFetchScope &collectionFetchScope();

    /*!
     * Sets the tag fetch scope.
     *
     * Controls how much of an tag's data is fetched from the server.
     *
     * \a fetchScope The new scope for tag fetch operations.
     *
     * \sa tagFetchScope()
     */
    void setTagFetchScope(const TagFetchScope &fetchScope);

    /*!
     * Returns the tag fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable.
     *
     * Returns a reference to the current tag fetch scope
     *
     * \sa setTagFetchScope() for replacing the current tag fetch scope
     */
    TagFetchScope &tagFetchScope();

    /*!
     * Returns the list of collections being monitored.
     *
     * \since 4.3
     */
    [[nodiscard]] Collection::List collectionsMonitored() const;

    /*!
     * Returns the set of items being monitored.
     *
     * Faster version (at least on 32-bit systems) of itemsMonitored().
     *
     * \since 4.6
     */
    [[nodiscard]] QList<Item::Id> itemsMonitoredEx() const;

    /*!
     * Returns the number of items being monitored.
     * Optimization.
     * \since 4.14.3
     */
    [[nodiscard]] int numItemsMonitored() const;

    /*!
     * Returns the set of mimetypes being monitored.
     *
     * \since 4.3
     */
    [[nodiscard]] QStringList mimeTypesMonitored() const;

    /*!
     * Returns the number of mimetypes being monitored.
     * Optimization.
     * \since 4.14.3
     */
    [[nodiscard]] int numMimeTypesMonitored() const;

    /*!
     * Returns the set of tags being monitored.
     *
     * \since 4.13
     */
    [[nodiscard]] QList<Tag::Id> tagsMonitored() const;

    /*!
     * Returns the set of types being monitored.
     *
     * \since 4.13
     */
    [[nodiscard]] QList<Type> typesMonitored() const;

    /*!
     * Returns the set of identifiers for resources being monitored.
     *
     * \since 4.3
     */
    [[nodiscard]] QList<QByteArray> resourcesMonitored() const;

    /*!
     * Returns the number of resources being monitored.
     * Optimization.
     * \since 4.14.3
     */
    [[nodiscard]] int numResourcesMonitored() const;

    /*!
     * Returns true if everything is being monitored.
     *
     * \since 4.3
     */
    [[nodiscard]] bool isAllMonitored() const;

    /*!
     * Sets the session used by the Monitor to communicate with the %Akonadi server.
     * If not set, the Akonadi::Session::defaultSession is used.
     * \a session the session to be set
     * \since 4.4
     */
    void setSession(Akonadi::Session *session);

    /*!
     * Returns the Session used by the monitor to communicate with Akonadi.
     *
     * \since 4.4
     */
    [[nodiscard]] Session *session() const;

    /*!
     * Allows to enable/disable collection move translation. If enabled (the default), move
     * notifications are automatically translated into add/remove notifications if the source/destination
     * is outside of the monitored collection hierarchy.
     * \a enabled enables collection move translation if set as \\ true
     * \since 4.9
     */
    void setCollectionMoveTranslationEnabled(bool enabled);

Q_SIGNALS:
    /*!
     * This signal is emitted if a monitored item has changed, e.g. item parts have been modified.
     *
     * \a item The changed item.
     * \a partIdentifiers The identifiers of the item parts that has been changed.
     */
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers);

    /*!
     * This signal is emitted if flags of monitored items have changed.
     *
     * \a items Items that were changed
     * \a addedFlags Flags that have been added to each item in \a items
     * \a removedFlags Flags that have been removed from each item in \a items
     * \since 4.11
     */
    void itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags);

    /*!
     * This signal is emitted if tags of monitored items have changed.
     *
     * \a items Items that were changed
     * \a addedTags Tags that have been added to each item in \a items.
     * \a removedTags Tags that have been removed from each item in \a items
     * \since 4.13
     */
    void itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags);

    /*!
     * This signal is emitted if a monitored item has been moved between two collections
     *
     * \a item The moved item.
     * \a collectionSource The collection the item has been moved from.
     * \a collectionDestination The collection the item has been moved to.
     */
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination);

    /*!
     * This is signal is emitted when multiple monitored items have been moved between two collections
     *
     * \a items Moved items
     * \a collectionSource The collection the items have been moved from.
     * \a collectionDestination The collection the items have been moved to.
     *
     * \since 4.11
     */
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination);

    /*!
     * This signal is emitted if an item has been added to a monitored collection in the Akonadi storage.
     *
     * \a item The new item.
     * \a collection The collection the item has been added to.
     */
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection);

    /*!
     * This signal is emitted if
     *   - a monitored item has been removed from the Akonadi storage
     * or
     *   - a item has been removed from a monitored collection.
     *
     * \a item The removed item.
     */
    void itemRemoved(const Akonadi::Item &item);

    /*!
     * This signal is emitted if monitored items have been removed from Akonadi
     * storage of items have been removed from a monitored collection.
     *
     * \a items Removed items
     *
     * \since 4.11
     */
    void itemsRemoved(const Akonadi::Item::List &items);

    /*!
     * This signal is emitted if a reference to an item is added to a virtual collection.
     * \a item The linked item.
     * \a collection The collection the item is linked to.
     *
     * \since 4.2
     */
    void itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection);

    /*!
     * This signal is emitted if a reference to multiple items is added to a virtual collection
     *
     * \a items The linked items
     * \a collection The collections the items are linked to
     *
     * \since 4.11
     */
    void itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection);

    /*!
     * This signal is emitted if a reference to an item is removed from a virtual collection.
     * \a item The unlinked item.
     * \a collection The collection the item is unlinked from.
     *
     * \since 4.2
     */
    void itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection);

    /*!
     * This signal is emitted if a reference to items is removed from a virtual collection
     *
     * \a items The unlinked items
     * \a collection The collections the items are unlinked from
     *
     * \since 4.11
     */
    void itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection);

    /*!
     * This signal is emitted if a new collection has been added to a monitored collection in the Akonadi storage.
     *
     * \a collection The new collection.
     * \a parent The parent collection.
     */
    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent);

    /*!
     * This signal is emitted if a monitored collection has been changed (properties or content).
     *
     * \a collection The changed collection.
     */
    void collectionChanged(const Akonadi::Collection &collection);

    /*!
     * This signal is emitted if a monitored collection has been changed (properties or attributes).
     *
     * \a collection The changed collection.
     * \a attributeNames The names of the collection attributes that have been changed.
     *
     * \since 4.4
     */
    void collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &attributeNames);

    /*!
     * This signals is emitted if a monitored collection has been moved.
     *
     * \a collection The moved collection.
     * \a source The previous parent collection.
     * \a destination The new parent collection.
     *
     * \since 4.4
     */
    void collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &destination);

    /*!
     * This signal is emitted if a monitored collection has been removed from the Akonadi storage.
     *
     * \a collection The removed collection.
     */
    void collectionRemoved(const Akonadi::Collection &collection);

    /*!
     * This signal is emitted if a collection has been subscribed to by the user.
     *  It will be emitted even for unmonitored collections as the check for whether to
     *  monitor it has not been applied yet.
     *
     * \a collection The subscribed collection
     * \a parent The parent collection of the subscribed collection.
     *
     * \since 4.6
     */
    void collectionSubscribed(const Akonadi::Collection &collection, const Akonadi::Collection &parent);

    /*!
     * This signal is emitted if a user unsubscribes from a collection.
     *
     * \a collection The unsubscribed collection
     *
     * \since 4.6
     */
    void collectionUnsubscribed(const Akonadi::Collection &collection);

    /*!
     * This signal is emitted if the statistics information of a monitored collection
     * has changed.
     *
     * \a id The collection identifier of the changed collection.
     * \a statistics The updated collection statistics, invalid if automatic
     *                   fetching of statistics changes is disabled.
     */
    void collectionStatisticsChanged(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &statistics);

    /*!
     * This signal is emitted if a tag has been added to Akonadi storage.
     *
     * \a tag The added tag
     * \since 4.13
     */
    void tagAdded(const Akonadi::Tag &tag);

    /*!
     * This signal is emitted if a monitored tag is changed on the server.
     *
     * \a tag The changed tag.
     * \since 4.13
     */
    void tagChanged(const Akonadi::Tag &tag);

    /*!
     * This signal is emitted if a monitored tag is removed from the server storage.
     *
     * The monitor will also emit itemTagsChanged() signal for all monitored items
     * (if any) that were tagged by \a tag.
     *
     * \a tag The removed tag.
     * \since 4.13
     */
    void tagRemoved(const Akonadi::Tag &tag);

    /*!
     * This signal is emitted when Subscribers are monitored and a new subscriber
     * subscribers to the server.
     *
     * \a subscriber The new subscriber
     * \since 5.4
     *
     * \
ote Monitoring for subscribers and listening to this signal only makes
     * sense if you want to globally debug Monitors. There is no reason to use
     * this in regular applications.
     */
    void notificationSubscriberAdded(const Akonadi::NotificationSubscriber &subscriber);

    /*!
     * This signal is emitted when Subscribers are monitored and an existing
     * subscriber changes its subscription.
     *
     * \a subscriber The changed subscriber
     * \since 5.4
     *
     * \
ote Monitoring for subscribers and listening to this signal only makes
     * sense if you want to globally debug Monitors. There is no reason to use
     * this in regular applications.
     */
    void notificationSubscriberChanged(const Akonadi::NotificationSubscriber &subscriber);

    /*!
     * This signal is emitted when Subscribers are monitored and an existing
     * subscriber unsubscribes from the server.
     *
     * \a subscriber The removed subscriber
     * \since 5.4
     *
     * \
ote Monitoring for subscribers and listening to this signal only makes
     * sense if you want to globally debug Monitors. There is no reason to use
     * this in regular applications.
     */
    void notificationSubscriberRemoved(const Akonadi::NotificationSubscriber &subscriber);

    /*!
     * This signal is emitted when Notifications are monitored and the server emits
     * any change notification.
     *
     * \since 5.4
     *
     * \
ote Getting introspection into all change notifications only makes sense
     * if you want to globally debug Notifications. There is no reason to use
     * this in regular applications.
     */
    void debugNotification(const Akonadi::ChangeNotification &notification);

    /*!
     * This signal is emitted if the Monitor starts or stops monitoring \a collection explicitly.
     * \a collection The collection
     * \a monitored Whether the collection is now being monitored or not.
     *
     * \since 4.3
     */
    void collectionMonitored(const Akonadi::Collection &collection, bool monitored);

    /*!
     * This signal is emitted if the Monitor starts or stops monitoring \a item explicitly.
     * \a item The item
     * \a monitored Whether the item is now being monitored or not.
     *
     * \since 4.3
     */
    void itemMonitored(const Akonadi::Item &item, bool monitored);

    /*!
     * This signal is emitted if the Monitor starts or stops monitoring the resource with the identifier \a identifier explicitly.
     * \a identifier The identifier of the resource.
     * \a monitored Whether the resource is now being monitored or not.
     *
     * \since 4.3
     */
    void resourceMonitored(const QByteArray &identifier, bool monitored);

    /*!
     * This signal is emitted if the Monitor starts or stops monitoring \a mimeType explicitly.
     * \a mimeType The mimeType.
     * \a monitored Whether the mimeType is now being monitored or not.
     *
     * \since 4.3
     */
    void mimeTypeMonitored(const QString &mimeType, bool monitored);

    /*!
     * This signal is emitted if the Monitor starts or stops monitoring everything.
     * \a monitored Whether everything is now being monitored or not.
     *
     * \since 4.3
     */
    void allMonitored(bool monitored);

    /*!
     * This signal is emitted if the Monitor starts or stops monitoring \a tag explicitly.
     * \a tag The tag.
     * \a monitored Whether the tag is now being monitored or not.
     * \since 4.13
     */
    void tagMonitored(const Akonadi::Tag &tag, bool monitored);

    /*!
     * This signal is emitted if the Monitor starts or stops monitoring \a type explicitly
     * \a type The type.
     * \a monitored Whether the type is now being monitored or not.
     * \since 4.13
     */
    void typeMonitored(const Akonadi::Monitor::Type type, bool monitored);

    void monitorReady();

protected:
    void connectNotify(const QMetaMethod &signal) override;
    void disconnectNotify(const QMetaMethod &signal) override;

    friend class EntityTreeModel;
    friend class EntityTreeModelPrivate;
    std::unique_ptr<MonitorPrivate> const d_ptr;
    explicit Monitor(MonitorPrivate *d, QObject *parent = nullptr);

private:
    Q_DECLARE_PRIVATE(Monitor)

    Q_PRIVATE_SLOT(d_ptr, void handleCommands())
    Q_PRIVATE_SLOT(d_ptr, void invalidateCollectionCache(qint64))
    Q_PRIVATE_SLOT(d_ptr, void invalidateItemCache(qint64))
    Q_PRIVATE_SLOT(d_ptr, void invalidateTagCache(qint64))

    friend class ResourceBasePrivate;
};

}
