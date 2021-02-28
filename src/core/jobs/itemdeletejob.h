/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ITEMDELETEJOB_H
#define AKONADI_ITEMDELETEJOB_H

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"

namespace Akonadi
{
class Collection;
class ItemDeleteJobPrivate;

/**
 * @short Job that deletes items from the Akonadi storage.
 *
 * This job removes the given items from the Akonadi storage.
 *
 * Example:
 *
 * @code
 *
 * const Akonadi::Item item = ...
 *
 * ItemDeleteJob *job = new ItemDeleteJob(item);
 * connect(job, SIGNAL(result(KJob*)), this, SLOT(deletionResult(KJob*)));
 *
 * @endcode
 *
 * Example:
 *
 * @code
 *
 * const Akonadi::Item::List items = ...
 *
 * ItemDeleteJob *job = new ItemDeleteJob(items);
 * connect(job, SIGNAL(result(KJob*)), this, SLOT(deletionResult(KJob*)));
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ItemDeleteJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new item delete job that deletes @p item. The item
     * needs to have a unique identifier set.
     *
     * @internal
     * For internal use only, the item may have a remote identifier set instead
     * of a unique identifier. In this case, a collection or resource context
     * needs to be selected using ResourceSelectJob.
     * @endinternal
     *
     * @param item The item to delete.
     * @param parent The parent object.
     */
    explicit ItemDeleteJob(const Item &item, QObject *parent = nullptr);

    /**
     * Creates a new item delete job that deletes all items in the list
     * @p items. Each item needs to have a unique identifier set. These items
     * can be located in any collection.
     *
     * @internal
     * For internal use only, the items may have remote identifiers set instead
     * of unique identifiers. In this case, a collection or resource context
     * needs to be selected using ResourceSelectJob.
     * @endinternal
     *
     * @param items The items to delete.
     * @param parent The parent object.
     *
     * @since 4.3
     */
    explicit ItemDeleteJob(const Item::List &items, QObject *parent = nullptr);

    /**
     * Creates a new item delete job that deletes all items in the collection
     * @p collection. The collection needs to have a unique identifier set.
     *
     * @internal
     * For internal use only, the collection may have a remote identifier set
     * instead of a unique identifier. In this case, a resource context needs
     * to be selected using ResourceSelectJob.
     * @endinternal
     *
     * @param collection The collection which content should be deleted.
     * @param parent The parent object.
     *
     * @since 4.3
     */
    explicit ItemDeleteJob(const Collection &collection, QObject *parent = nullptr);

    /**
     * Creates a new item delete job that deletes all items that have assigned
     * the tag @p tag.
     *
     * @param tag The tag which content should be deleted.
     * @param parent The parent object.
     *
     * @since 4.14
     */
    explicit ItemDeleteJob(const Tag &tag, QObject *parent = nullptr);

    /**
     * Destroys the item delete job.
     */
    ~ItemDeleteJob() override;

    /**
     * Returns the items passed on in the constructor.
     * @since 4.4
     */
    Q_REQUIRED_RESULT Item::List deletedItems() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    /// @cond PRIVATE
    Q_DECLARE_PRIVATE(ItemDeleteJob)
    /// @endcond
};

}

#endif
