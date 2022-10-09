/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"
namespace Akonadi
{
class Collection;
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
    ItemMoveJob(const Item &item, const Collection &destination, QObject *parent = nullptr);

    /**
     * Move the given items into @p destination.
     *
     * @param items A list of items to move.
     * @param destination The destination collection.
     * @param parent The parent object.
     */
    ItemMoveJob(const Item::List &items, const Collection &destination, QObject *parent = nullptr);

    /**
     * Move the given items from @p source to @p destination.
     *
     * @internal If the items are identified only by RID, then you MUST use this
     * constructor to specify the source collection, otherwise the job will fail.
     * RID-based moves are only allowed to resources.
     *
     * @since 4.14
     */
    ItemMoveJob(const Item::List &items, const Collection &source, const Collection &destination, QObject *parent = nullptr);

    /**
     * Destroys the item move job.
     */
    ~ItemMoveJob() override;

    /**
     * Returns the destination collection.
     *
     * @since 4.7
     */
    Q_REQUIRED_RESULT Collection destinationCollection() const;

    /**
     * Returns the list of items that where passed in the constructor.
     *
     * @since 4.7
     */
    Q_REQUIRED_RESULT Akonadi::Item::List items() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(ItemMoveJob)
};

}
