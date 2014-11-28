/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMMOVEJOB_H
#define AKONADI_ITEMMOVEJOB_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi {

class Collection;
class Item;
class ItemMoveJobPrivate;

/**
 * @short Job that moves an item into a different collection in the Akonadi storage.
 *
 * This job takes an item and moves it to a collection in the Akonadi storage.
 *
 * @code
 *
 * Akonadi::Item item = ...
 * Akonadi::Collection collection = ...
 *
 * Akonadi::ItemMoveJob *job = new Akonadi::ItemMoveJob( item, collection );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(moveResult(KJob*)) );
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ItemMoveJob : public Job
{
    Q_OBJECT

public:
    /**
     * Move the given item into the given collection.
     *
     * @param item The item to move.
     * @param destination The destination collection.
     * @param parent The parent object.
     */
    ItemMoveJob(const Item &item, const Collection &destination, QObject *parent = 0);

    /**
     * Move the given items into @p destination.
     *
     * @param items A list of items to move.
     * @param destination The destination collection.
     * @param parent The parent object.
     */
    ItemMoveJob(const QList<Item> &items, const Collection &destination, QObject *parent = 0);

    /**
     * Move the given items from @p sourec to @p destination.
     *
     * @internal If the items are identified only by RID, then you MUST use this
     * constructor to specify the source collection, otherwise the job will fail.
     * RID-based moves are only allowed to resources.
     *
     * @since 4.14
     */
    ItemMoveJob(const QList<Item> &items, const Collection &source, const Collection &destination, QObject *parent = 0);

    /**
     * Destroys the item move job.
     */
    ~ItemMoveJob();

    /**
     * Returns the destination collection.
     *
     * @since 4.7
     */
    Collection destinationCollection() const;

    /**
     * Returns the list of items that where passed in the constructor.
     *
     * @since 4.7
     */
    QList<Item> items() const;

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(ItemMoveJob)
    template <typename T, typename MoveJob> friend class MoveJobImpl;
};

}

#endif
