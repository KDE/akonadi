/*
 *    Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *    This library is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU Library General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at your
 *    option) any later version.
 *
 *    This library is distributed in the hope that it will be useful, but WITHOUT
 *    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *    License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to the
 *    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *    02110-1301, USA.
 */

#ifndef AKONADI_TRASHRESTOREJOB_H
#define AKONADI_TRASHRESTOREJOB_H

#include "akonadicore_export.h"
#include "job.h"
#include "item.h"
#include "collection.h"

namespace Akonadi
{

/**
 * @short Job that restores entites from trash
 *
 * This job restores the given entites from trash.
 * The EntityDeletedAttribute is removed and the item is restored to the stored restore collection.
 *
 * If the stored restore collection is not available, the root collection of the original resource is used.
 * If also this is not available, setTargetCollection has to be used to restore the item to a specific collection.
 *
 * Example:
 *
 * @code
 *
 * const Akonadi::Item::List items = ...
 *
 * TrashRestoreJob *job = new TrashRestoreJob( items );
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(restoreResult(KJob*)) );
 *
 * @endcode
 *
 * @author Christian Mollekopf <chrigi_1@fastmail.fm>
 * @since 4.8
 */
class AKONADICORE_EXPORT TrashRestoreJob : public Job
{
    Q_OBJECT
public:

    /**
     * All items need to be from the same resource
     */
    explicit TrashRestoreJob(const Item &item, QObject *parent = Q_NULLPTR);

    explicit TrashRestoreJob(const Item::List &items, QObject *parent = Q_NULLPTR);

    explicit TrashRestoreJob(const Collection &collection, QObject *parent = Q_NULLPTR);

    ~TrashRestoreJob();

    /**
     * Sets the target collection, where the item is moved to.
     * If not set the item will be restored in the collection saved in the EntityDeletedAttribute.
     * @param collection the collection to set as target
     */
    void setTargetCollection(const Collection collection);

    Item::List items() const;
protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class TrashRestoreJobPrivate;
    Q_DECLARE_PRIVATE(TrashRestoreJob)
    Q_PRIVATE_SLOT(d_func(), void selectResult(KJob *))
    Q_PRIVATE_SLOT(d_func(), void targetCollectionFetched(KJob *))
    Q_PRIVATE_SLOT(d_func(), void removeAttribute(const Akonadi::Collection::List &))
    Q_PRIVATE_SLOT(d_func(), void removeAttribute(const Akonadi::Item::List &))
    Q_PRIVATE_SLOT(d_func(), void collectionsReceived(const Akonadi::Collection::List &))
    Q_PRIVATE_SLOT(d_func(), void itemsReceived(const Akonadi::Item::List &))
    //@endcond
};

}

#endif
