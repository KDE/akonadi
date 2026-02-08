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

/*!
 * \brief Syncs between items known to a client (usually a resource) and the Akonadi storage.
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
 * \
ote This is provided for convenience to implement "save all" like behavior,
 *       however it is strongly recommended to use single item jobs whenever
 *       possible, e.g. ItemCreateJob, ItemModifyJob and ItemDeleteJob
 *
 * \class Akonadi::ItemSync
 * \inheaderfile Akonadi/ItemSync
 * \inmodule AkonadiCore
 *
 * \author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT ItemSync : public Job
{
    Q_OBJECT

public:
    enum MergeMode {
        RIDMerge,
        GIDMerge,
    };

    /*!
     * Creates a new item synchronizer.
     *
     * \a collection The collection we are syncing.
     * \a timestamp Optional timestamp of itemsync start. Will be used to detect local changes that happen
                        while the ItemSync is running.
     * \a parent The parent object.
     */
    explicit ItemSync(const Collection &collection, const QDateTime &timestamp = {}, QObject *parent = nullptr);

    /*!
     * Destroys the item synchronizer.
     */
    ~ItemSync() override;

    /*!
     * Sets the full item list for the collection.
     *
     * Usually the result of a full item listing.
     *
     * \warning If the client using this is a resource, all items must have
     *          a valid remote identifier.
     *
     * \a items A list of items.
     */
    void setFullSyncItems(const Item::List &items);

    /*!
     * Set the amount of items which you are going to return in total
     * by using the setFullSyncItems()/setIncrementalSyncItems() methods.
     *
     * \warning By default the item sync will automatically end once
     * sufficient items have been provided.
     * To disable this use setDisableAutomaticDeliveryDone
     *
     * \sa setDisableAutomaticDeliveryDone
     * \a amount The amount of items in total.
     */
    void setTotalItems(int amount);

    /*!
      Enable item streaming. Item streaming means that the items delivered by setXItems() calls
      are delivered in chunks and you manually indicate when all items have been delivered
      by calling deliveryDone().
      \a enable \\ true to enable item streaming
    */
    void setStreamingEnabled(bool enable);

    /*!
      Notify ItemSync that all remote items have been delivered.
      Only call this in streaming mode.
    */
    void deliveryDone();

    /*!
     * Sets the item lists for incrementally syncing the collection.
     *
     * Usually the result of an incremental remote item listing.
     *
     * \warning If the client using this is a resource, all items must have
     *          a valid remote identifier.
     *
     * \a changedItems A list of items added or changed by the client.
     * \a removedItems A list of items deleted by the client.
     */
    void setIncrementalSyncItems(const Item::List &changedItems, const Item::List &removedItems);

    /*!
     * Aborts the sync process and rolls back all not yet committed transactions.
     * Use this if an external error occurred during the sync process (such as the
     * user canceling it).
     * \since 4.5
     */
    void rollback();

    /*!
     * Transaction mode used by ItemSync.
     * \since 4.6
     */
    enum TransactionMode {
        SingleTransaction, ///< Use a single transaction for the entire sync process (default), provides maximum consistency ("all or nothing") and best
                           ///< performance
        MultipleTransactions, ///< Use one transaction per chunk of delivered items, good compromise between the other two when using streaming
        NoTransaction ///< Use no transaction at all, provides highest responsiveness (might therefore feel faster even when actually taking slightly longer),
                      ///< no consistency guaranteed (can fail anywhere in the sync process)
    };

    /*!
     * Set the transaction mode to use for this sync.
     * \
ote You must call this method before starting the sync, changes afterwards lead to undefined results.
     * \a mode the transaction mode to use
     * \since 4.6
     */
    void setTransactionMode(TransactionMode mode);

    /*!
     * Minimum number of items required to start processing in streaming mode.
     * When MultipleTransactions is used, one transaction per batch will be created.
     *
     * \sa setBatchSize()
     * \since 4.14
     */
    [[nodiscard]] int batchSize() const;

    /*!
     * Set the batch size.
     *
     * The default is 10.
     *
     * \
ote You must call this method before starting the sync, changes afterwards lead to undefined results.
     * \sa batchSize()
     * \since 4.14
     */
    void setBatchSize(int);

    /*!
     * Disables the automatic completion of the item sync,
     * based on the number of delivered items.
     *
     * This ensures that the item sync only finishes once deliveryDone()
     * is called, while still making it possible to use the progress
     * reporting of the ItemSync.
     *
     * \
ote You must call this method before starting the sync, changes afterwards lead to undefined results.
     * \sa setTotalItems
     * \since 4.14
     */
    void setDisableAutomaticDeliveryDone(bool disable);

    /*!
     * Returns current merge mode
     *
     * \sa setMergeMode()
     * \since 5.1
     */
    [[nodiscard]] MergeMode mergeMode() const;

    /*!
     * Set what merge method should be used for next ItemSync run
     *
     * By default ItemSync uses RIDMerge method.
     *
     * See ItemCreateJob for details on Item merging.
     *
     * \
ote You must call this method before starting the sync, changes afterwards lead to undefined results.
     * \sa mergeMode
     * \since 4.14.11
     */
    void setMergeMode(MergeMode mergeMode);

Q_SIGNALS:
    /*!
     * Signals the resource that new items can be delivered.
     * \a remainingBatchSize the number of items required to complete the batch (typically the same as batchSize())
     *
     * \since 4.14
     */
    void readyForNextBatch(int remainingBatchSize);

    /*!
     * \internal
     * Emitted whenever a transaction is committed. This is for testing only.
     *
     * \since 4.14
     */
    void transactionCommitted();

protected:
    void doStart() override;
    void slotResult(KJob *job) override;

private:
    Q_DECLARE_PRIVATE(ItemSync)

    Q_PRIVATE_SLOT(d_func(), void slotLocalListDone(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotTransactionResult(KJob *))
    Q_PRIVATE_SLOT(d_func(), void slotItemsReceived(const Akonadi::Item::List &))
};

}
