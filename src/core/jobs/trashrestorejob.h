/*
 *    SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef AKONADI_TRASHRESTOREJOB_H
#define AKONADI_TRASHRESTOREJOB_H

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"
#include "job.h"

namespace Akonadi
{
/**
 * @short Job that restores entities from trash
 *
 * This job restores the given entities from trash.
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
    explicit TrashRestoreJob(const Item &item, QObject *parent = nullptr);

    explicit TrashRestoreJob(const Item::List &items, QObject *parent = nullptr);

    explicit TrashRestoreJob(const Collection &collection, QObject *parent = nullptr);

    ~TrashRestoreJob() override;

    /**
     * Sets the target collection, where the item is moved to.
     * If not set the item will be restored in the collection saved in the EntityDeletedAttribute.
     * @param collection the collection to set as target
     */
    void setTargetCollection(const Collection &collection);

    Q_REQUIRED_RESULT Item::List items() const;

protected:
    void doStart() override;

private:
    //@cond PRIVATE
    class TrashRestoreJobPrivate;
    Q_DECLARE_PRIVATE(TrashRestoreJob)
    //@endcond
};

}

#endif
