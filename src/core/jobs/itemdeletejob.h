/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMDELETEJOB_H
#define AKONADI_ITEMDELETEJOB_H

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"

namespace Akonadi {

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
     * needs to be selected using CollectionSelectJob or ResourceSelectJob.
     * @endinternal
     *
     * @param item The item to delete.
     * @param parent The parent object.
     */
    explicit ItemDeleteJob(const Item &item, QObject *parent = Q_NULLPTR);

    /**
     * Creates a new item delete job that deletes all items in the list
     * @p items. Each item needs to have a unique identifier set. These items
     * can be located in any collection.
     *
     * @internal
     * For internal use only, the items may have remote identifiers set instead
     * of unique identifiers. In this case, a collection or resource context
     * needs to be selected using CollectionSelectJob or ResourceSelectJob.
     * @endinternal
     *
     * @param items The items to delete.
     * @param parent The parent object.
     *
     * @since 4.3
     */
    explicit ItemDeleteJob(const Item::List &items, QObject *parent = Q_NULLPTR);

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
    explicit ItemDeleteJob(const Collection &collection, QObject *parent = Q_NULLPTR);

    /**
     * Creates a new item delete job that deletes all items that have assigned
     * the tag @p tag.
     *
     * @param tag The tag which content should be deleted.
     * @param parent The parent object.
     *
     * @since 4.14
     */
    explicit ItemDeleteJob(const Tag &tag, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the item delete job.
     */
    ~ItemDeleteJob();

    /**
     * Returns the items passed on in the constructor.
     * @since 4.4
     */
    Item::List deletedItems() const;

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(ItemDeleteJob)
    //@endcond
};

}

#endif
