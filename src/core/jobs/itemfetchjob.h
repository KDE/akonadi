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

#ifndef AKONADI_ITEMFETCHJOB_H
#define AKONADI_ITEMFETCHJOB_H

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"

namespace Akonadi {

class Collection;
class ItemFetchJobPrivate;
class ItemFetchScope;

/**
 * @short Job that fetches items from the Akonadi storage.
 *
 * This class is used to fetch items from the Akonadi storage.
 * Which parts of the items (e.g. headers only, attachments or all)
 * can be specified by the ItemFetchScope.
 *
 * Note that ItemFetchJob does not refresh the Akonadi storage from the
 * backend; this is unnecessary due to the fact that backend updates
 * automatically trigger an update to the Akonadi database whenever they occur
 * (unless the resource is offline).
 *
 * Note that items can not be created in the root collection (Collection::root())
 * and therefore can not be fetched from there either. That is - an item fetch in
 * the root collection will yield an empty list.
 *
 *
 * Example:
 *
 * @code
 *
 * // Fetch all items with full payload from a collection
 *
 * const Collection collection = getCollection();
 *
 * Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(collection);
 * connect(job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)));
 * job->fetchScope().fetchFullPayload();
 *
 * ...
 *
 * MyClass::jobFinished(KJob *job)
 * {
 *   if (job->error()) {
 *     qDebug() << "Error occurred";
 *     return;
 *   }
 *
 *   Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>(job);
 *
 *   const Akonadi::Item::List items = fetchJob->items();
 *   foreach (const Akonadi::Item &item, items) {
 *     qDebug() << "Item ID:" << item.id();
 *   }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ItemFetchJob : public Job
{
    Q_OBJECT
    Q_FLAGS(DeliveryOptions)
public:
    /**
     * Creates a new item fetch job that retrieves all items inside the given collection.
     *
     * @param collection The parent collection to fetch all items from.
     * @param parent The parent object.
     */
    explicit ItemFetchJob(const Collection &collection, QObject *parent = Q_NULLPTR);

    /**
     * Creates a new item fetch job that retrieves the specified item.
     * If the item has a uid set, this is used to identify the item on the Akonadi
     * server. If only a remote identifier is available, that is used.
     * However, as remote identifiers are not necessarily globally unique, you
     * need to specify the collection to search in in that case, using
     * setCollection().
     *
     * @internal
     * For internal use only when using remote identifiers, the resource search
     * context can be set globally by ResourceSelectJob.
     * @endinternal
     *
     * @param item The item to fetch.
     * @param parent The parent object.
     */
    explicit ItemFetchJob(const Item &item, QObject *parent = Q_NULLPTR);

    /**
     * Creates a new item fetch job that retrieves the specified items.
     * If the items have a uid set, this is used to identify the item on the Akonadi
     * server. If only a remote identifier is available, that is used.
     * However, as remote identifiers are not necessarily globally unique, you
     * need to specify the collection to search in in that case, using
     * setCollection().
     *
     * @internal
     * For internal use only when using remote identifiers, the resource search
     * context can be set globally by ResourceSelectJob.
     * @endinternal
     *
     * @param items The items to fetch.
     * @param parent The parent object.
     * @since 4.4
     */
    explicit ItemFetchJob(const Item::List &items, QObject *parent = Q_NULLPTR);

    /**
     * Convenience ctor equivalent to ItemFetchJob(const Item::List &items, QObject *parent = Q_NULLPTR)
     * @since 4.8
     */
    explicit ItemFetchJob(const QList<Item::Id> &items, QObject *parent = Q_NULLPTR);


    /**
     * Creates a new item fetch job that retrieves all items tagged with specified @p tag.
     *
     * @param tag The tag to fetch all items from.
     * @param parent The parent object.
     *
     * @since 4.14
     */
    explicit ItemFetchJob(const Tag &tag, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the item fetch job.
     */
    virtual ~ItemFetchJob();

    /**
     * Returns the fetched items.
     *
     * This returns an empty list when not using the ItemGetter DeliveryOption.
     *
     * @note The items are invalid before the result(KJob*)
     *       signal has been emitted or if an error occurred.
     */
    Item::List items() const;

    /**
     * Save memory by clearing the fetched items.
     * @since 4.12
     */
    void clearItems();

    /**
     * Sets the item fetch scope.
     *
     * The ItemFetchScope controls how much of an item's data is fetched
     * from the server, e.g. whether to fetch the full item payload or
     * only meta data.
     *
     * @param fetchScope The new scope for item fetch operations.
     *
     * @see fetchScope()
     * @since 4.4
     */
    void setFetchScope(const ItemFetchScope &fetchScope);

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
     * @see setFetchScope() for replacing the current item fetch scope
     */
    ItemFetchScope &fetchScope();

    /**
     * Specifies the collection the item is in.
     * This is only required when retrieving an item based on its remote id
     * which might not be unique globally.
     *
     * @internal
     * @see ResourceSelectJob (for internal use only)
     * @endinternal
     */
    void setCollection(const Collection &collection);

    enum DeliveryOption {
        ItemGetter = 0x1,            ///< items available through items()
        EmitItemsIndividually = 0x2, ///< emitted via signal upon reception
        EmitItemsInBatches = 0x4,    ///< emitted via signal in bulk (collected and emitted delayed via timer)
        Default = ItemGetter | EmitItemsInBatches
    };
    Q_DECLARE_FLAGS(DeliveryOptions, DeliveryOption)

    /**
     * Sets the mechanisms by which the items should be fetched
     * @since 4.13
     */
    void setDeliveryOption(DeliveryOptions options);

    /**
     * Returns the delivery options
     * @since 4.13
     */
    DeliveryOptions deliveryOptions() const;

    /**
     * Returns the total number of retrieved items.
     * This works also without the ItemGetter DeliveryOption.
     * @since 4.14
     */
    int count() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever new items have been fetched completely.
     *
     * @note This is an optimization; instead of waiting for the end of the job
     *       and calling items(), you can connect to this signal and get the items
     *       incrementally.
     *
     * @param items The fetched items.
     */
    void itemsReceived(const Akonadi::Item::List &items);

protected:
    void doStart() Q_DECL_OVERRIDE;
    void doHandleResponse(const QByteArray &tag, const QByteArray &data) Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(ItemFetchJob)

    //@cond PRIVATE
    Q_PRIVATE_SLOT(d_func(), void timeout())
    //@endcond
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Akonadi::ItemFetchJob::DeliveryOptions)

#endif
