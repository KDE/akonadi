/*
    Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#ifndef AKONADI_TRASHJOB_H
#define AKONADI_TRASHJOB_H

#include "akonadicore_export.h"
#include "item.h"
#include "collection.h"
#include "job.h"

namespace Akonadi
{

/**
 * @short Job that moves items/collection to trash.
 *
 * This job marks the given entites as trash and moves them to a given trash collection, if available.
 *
 * Priorities of trash collections are the following:
 * 1. keepTrashInCollection()
 * 2. setTrashCollection()
 * 3. configured collection in TrashSettings
 * 4. keep in collection if nothing is configured
 *
 * If the item is already marked as trash, it will be deleted instead
 * only if deleteIfInTrash() is set.
 * The entity is marked as trash with the EntityDeletedAttribute.
 *
 * The restore collection in the EntityDeletedAttribute is set the following way:
 * -If entites are not moved to trash -> no restore collection
 * -If collection is deleted -> also subentites get collection.parentCollection as restore collection
 * -If multiple items are deleted -> all items get their parentCollection as restore collection
 *
 * Example:
 *
 * @code
 *
 * const Akonadi::Item::List items = ...
 *
 * TrashJob *job = new TrashJob( items );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(deletionResult(KJob*)) );
 *
 * @endcode
 *
 * @author Christian Mollekopf <chrigi_1@fastmail.fm>
 * @since 4.8
 */
class AKONADICORE_EXPORT TrashJob : public Job
{
    Q_OBJECT

public:

    /**
     * Creates a new trash job that marks @p item as trash, and moves it to the configured trash collection.
     *
     * If @p keepTrashInCollection is set, the item will not be moved to the configured trash collection.
     *
     * @param item The item to mark as trash.
     * @param parent The parent object.
     */
    explicit TrashJob(const Item &item, QObject *parent = 0);

    /**
     * Creates a new trash job that marks all items in the list
     * @p items as trash, and moves it to the configured trash collection.
     * The items can be in different collections/resources and will still be moved to the correct trash colleciton.
     *
     * If @p keepTrashInCollection is set, the item will not be moved to the configured trash collection.
     *
     * @param items The items to mark as trash.
     * @param parent The parent object.
     */
    explicit TrashJob(const Item::List &items, QObject *parent = 0);

    /**
     * Creates a new trash job that marks @p collection as trash, and moves it to the configured trash collection.
     * The subentities of the collection are also marked as trash.
     *
     * If @p keepTrashInCollection is set, the item will not be moved to the configured trash collection.
     *
     * @param collection The collection to mark as trash.
     * @param parent The parent object.
     */
    explicit TrashJob(const Collection &collection, QObject *parent = 0);

    ~TrashJob();

    /**
     * Ignore configured Trash collections and keep all items local
     */
    void keepTrashInCollection(bool enable);

    /**
     * Moves all entities to the give collection
     */
    void setTrashCollection(const Collection &trashcollection);

    /**
     * Delete Items which are already in trash, instead of ignoring them
     */
    void deleteIfInTrash(bool enable);

    Item::List items() const;

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class TrashJobPrivate;
    Q_DECLARE_PRIVATE(TrashJob)
    Q_PRIVATE_SLOT(d_func(), void selectResult(KJob *))
    Q_PRIVATE_SLOT(d_func(), void setAttribute(const Akonadi::Collection::List &))
    Q_PRIVATE_SLOT(d_func(), void setAttribute(const Akonadi::Item::List &))
    Q_PRIVATE_SLOT(d_func(), void setAttribute(KJob *))
    Q_PRIVATE_SLOT(d_func(), void collectionsReceived(const Akonadi::Collection::List &))
    Q_PRIVATE_SLOT(d_func(), void itemsReceived(const Akonadi::Item::List &))
    Q_PRIVATE_SLOT(d_func(), void parentCollectionReceived(const Akonadi::Collection::List &))
    //@endcond
};

}

#endif
