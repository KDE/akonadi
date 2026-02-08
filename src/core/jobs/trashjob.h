/*
    SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"
#include "job.h"

namespace Akonadi
{
class TrashJobPrivate;

/*!
 * \brief Job that moves items/collection to trash.
 *
 * \class Akonadi::TrashJob
 * \inheaderfile Akonadi/TrashJob
 * \inmodule AkonadiCore
 *
 * This job marks the given entities as trash and moves them to a given trash collection, if available.
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
 * -If entities are not moved to trash -> no restore collection
 * -If collection is deleted -> also subentities get collection.parentCollection as restore collection
 * -If multiple items are deleted -> all items get their parentCollection as restore collection
 *
 * Example:
 *
 * \code
 *
 * const Akonadi::Item::List items = ...
 *
 * TrashJob *job = new TrashJob( items );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(deletionResult(KJob*)) );
 *
 * \endcode
 *
 * \author Christian Mollekopf <chrigi_1@fastmail.fm>
 * \since 4.8
 */
class AKONADICORE_EXPORT TrashJob : public Job
{
    Q_OBJECT

public:
    /*!
     * Creates a new trash job that marks \a item as trash, and moves it to the configured trash collection.
     *
     * If \a keepTrashInCollection is set, the item will not be moved to the configured trash collection.
     *
     * \a item The item to mark as trash.
     * \a parent The parent object.
     */
    explicit TrashJob(const Item &item, QObject *parent = nullptr);

    /*!
     * Creates a new trash job that marks all items in the list
     * \a items as trash, and moves it to the configured trash collection.
     * The items can be in different collections/resources and will still be moved to the correct trash collection.
     *
     * If \a keepTrashInCollection is set, the item will not be moved to the configured trash collection.
     *
     * \a items The items to mark as trash.
     * \a parent The parent object.
     */
    explicit TrashJob(const Item::List &items, QObject *parent = nullptr);

    /*!
     * Creates a new trash job that marks \a collection as trash, and moves it to the configured trash collection.
     * The subentities of the collection are also marked as trash.
     *
     * If \a keepTrashInCollection is set, the item will not be moved to the configured trash collection.
     *
     * \a collection The collection to mark as trash.
     * \a parent The parent object.
     */
    explicit TrashJob(const Collection &collection, QObject *parent = nullptr);

    ~TrashJob() override;

    /*!
     * Ignore configured Trash collections and keep all items local
     */
    void keepTrashInCollection(bool enable);

    /*!
     * Moves all entities to the give collection
     */
    void setTrashCollection(const Collection &trashcollection);

    /*!
     * Delete Items which are already in trash, instead of ignoring them
     */
    void deleteIfInTrash(bool enable);

    /*!
     */
    [[nodiscard]] Item::List items() const;

protected:
    void doStart() override;

private:
    Q_DECLARE_PRIVATE(TrashJob)
};

}
