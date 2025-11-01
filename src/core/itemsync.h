/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"

#include <QDateTime>

namespace Akonadi
{
class Collection;
class ItemSyncPrivate;
class ItemFetchScope;

/**
 * @short Syncs between items known to a client (usually a resource) and the Akonadi storage.
 *
 * Remote Id must only be set by the resource storing the item, other clients
 * should leave it empty, since the resource responsible for the target collection
 * will be notified about the addition and then create a suitable remote Id.
 *
 * There are two different forms of ItemSync usage:
 * - Full-Sync: meaning the client provides all valid items, i.e. any item not
 *   part of the list but currently stored in Akonadi will be removed
 * - Incremental-Sync: meaning the client provides two lists, one for items which
 *   are new or modified and one for items which should be removed. Any item not
 *   part of either list but currently stored in Akonadi will not be changed.
 *
 * @note This is provided for convenience to implement "save all" like behavior,
 *       however it is strongly recommended to use single item jobs whenever
 *       possible, e.g. ItemCreateJob, ItemModifyJob and ItemDeleteJob
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT ItemSync : public Job
{
    Q_OBJECT

public:
    enum MergeMode {
        RIDMerge,
        GIDMerge,
    };

    /**
     * Creates a new item synchronizer.
     *
     * @param collection The collection we are syncing.
     * @param timestamp Optional timestamp of itemsync start. Will be used to detect local changes that happen
                        while the ItemSync is running.
     * @param parent The parent object.
     */
    explicit ItemSync(const Collection &collection, const QDateTime &timestamp = {}, QObject *parent = nullptr);

    /**
     * Destroys the item synchronizer.
     */
    ~ItemSync() override;

    /**
     * Sets the full item list for the collection.
     *
     * Usually the result of a full item listing.
     *
     * @warning If the client using this is a resource, all items must have
     *          a valid remote identifier.
     *
     * @param items A list of items.
     */
    void setFullSyncItems(const Item::List &items);

    /**
     * Set the amount of items which you are going to return in total
     * by using the setFullSyncItems()/setIncrementalSyncItems() methods.
     *
     * @warning By default the item sync will automatically end once
     * sufficient items have been provided.
     * To disable this use setDisableAutomaticDeliveryDone
     *
     * @see setDisableAutomaticDeliveryDone
     * @param amount The amount of items in total.
     */
    void setTotalItems(int amount);

    /**
      Enable item streaming. Item streaming means that the items delivered by setXItems() calls
      are delivered in chunks and you manually indicate when all items have been delivered
      by calling deliveryDone().
      @param enable @c true to enable item streaming
    */
    void setStreamingEnabled(bool enable);

    /**
      Notify ItemSync that all remote items have been delivered.
      Only call this in streaming mode.
    */
    void deliveryDone();

    /**
     * Sets the item lists for incrementally syncing the collection.
     *
     * Usually the result of an incremental remote item listing.
     *
     * @warning If the client using this is a resource, all items must have
     *          a valid remote identifier.
     *
     * @param changedItems A list of items added or changed by the client.
     * @param removedItems A list of items deleted by the client.
     */
    void setIncrementalSyncItems(const Item::List &changedItems, const Item::List &removedItems);

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
     */
    QT_DEPRECATED_X("Calling this method has no effect anymore.")
    void setFetchScope(ItemFetchScope &fetchScope);

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
    QT_DEPRECATED_X("Calling this method has no effect anymore.")
    ItemFetchScope &fetchScope();

    /**
     * Aborts the sync process and rolls back all not yet committed transactions.
     * Use this if an external error occurred during the sync process (such as the
     * user canceling it).
     * @since 4.5
     */
    void rollback();

    /**
     * Transaction mode used by ItemSync.
     * @since 4.6
     */
    enum TransactionMode {
        SingleTransaction
            Q_DECL_ENUMERATOR_DEPRECATED_X("The enumerator has no meaning anymore."), ///< Use a single transaction for the entire sync process (default),
                                                                                      ///< provides maximum consistency ("all or nothing") and best performance
        MultipleTransactions
            Q_DECL_ENUMERATOR_DEPRECATED_X("The enumerator has no meaning anymore."), ///< Use one transaction per chunk of delivered items, good compromise
                                                                                      ///< between the other two when using streaming
        NoTransaction Q_DECL_ENUMERATOR_DEPRECATED_X(
            "The enumerator has no meaning anymore.") ///< Use no transaction at all, provides highest responsiveness (might therefore feel faster even when
                                                      ///< actually taking slightly longer), no consistency guaranteed (can fail anywhere in the sync process)
    };

    /**
     * Set the transaction mode to use for this sync.
     * @note You must call this method before starting the sync, changes afterwards lead to undefined results.
     * @param mode the transaction mode to use
     * @since 4.6
     */
    QT_DEPRECATED_X("Calling this method has no effect anymore.")
    void setTransactionMode(TransactionMode mode);

    /**
     * Minimum number of items required to start processing in streaming mode.
     * When MultipleTransactions is used, one transaction per batch will be created.
     *
     * @see setBatchSize()
     * @since 4.14
     */
    QT_DEPRECATED_X("Calling this method has no effect anymore.")
    [[nodiscard]] int batchSize() const;

    /**
     * Set the batch size.
     *
     * The default is 10.
     *
     * @note You must call this method before starting the sync, changes afterwards lead to undefined results.
     * @see batchSize()
     * @since 4.14
     */
    QT_DEPRECATED_X("Calling this method has no effect anymore.")
    void setBatchSize(int);

    /**
     * Disables the automatic completion of the item sync,
     * based on the number of delivered items.
     *
     * This ensures that the item sync only finishes once deliveryDone()
     * is called, while still making it possible to use the progress
     * reporting of the ItemSync.
     *
     * @note You must call this method before starting the sync, changes afterwards lead to undefined results.
     * @see setTotalItems
     * @since 4.14
     */
    void setDisableAutomaticDeliveryDone(bool disable);

    /**
     * Returns current merge mode
     *
     * @see setMergeMode()
     * @since 5.1
     */
    [[nodiscard]] MergeMode mergeMode() const;

    /**
     * Set what merge method should be used for next ItemSync run
     *
     * By default ItemSync uses RIDMerge method.
     *
     * See ItemCreateJob for details on Item merging.
     *
     * @note You must call this method before starting the sync, changes afterwards lead to undefined results.
     * @see mergeMode
     * @since 4.14.11
     */
    void setMergeMode(MergeMode mergeMode);

Q_SIGNALS:
    /**
     * Signals the resource that new items can be delivered.
     * @param remainingBatchSize the number of items required to complete the batch (typically the same as batchSize())
     *
     * @since 4.14
     */
    QT_DEPRECATED_X("This signal is no longer used. It's only emitted once for backwards compatibility.")
    void readyForNextBatch(int remainingBatchSize);

    /**
     * @internal
     * Emitted whenever a transaction is committed. This is for testing only.
     *
     * @since 4.14
     */
    QT_DEPRECATED_X("This signal is no longer emitted.")
    void transactionCommitted();

protected:
    void doStart() override;
    void slotResult(KJob *job) override;

private:
    /// @cond PRIVATE
    Q_DECLARE_PRIVATE(ItemSync)
    //@endcond
};

} // namespace Akonadi
