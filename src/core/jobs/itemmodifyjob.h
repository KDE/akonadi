/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMMODIFYJOB_H
#define AKONADI_ITEMMODIFYJOB_H

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"

namespace Akonadi {

class ItemModifyJobPrivate;

/**
 * @short Job that modifies an existing item in the Akonadi storage.
 *
 * This job is used to writing back items to the Akonadi storage, after
 * the user has changed them in any way.
 * For performance reasons either the full item (including the full payload)
 * can written back or only the meta data of the item.
 *
 * Example:
 *
 * @code
 *
 * // Fetch item with unique id 125
 * Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob( Akonadi::Item( 125 ) );
 * connect( fetchJob, SIGNAL(result(KJob*)), SLOT(fetchFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::fetchFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     return;
 *
 *   Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
 *
 *   Akonadi::Item item = fetchJob->items().first();
 *
 *   // Set a custom flag
 *   item.setFlag( "\GotIt" );
 *
 *   // Store back modified item
 *   Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob( item );
 *   connect( modifyJob, SIGNAL(result(KJob*)), SLOT(modifyFinished(KJob*)) );
 * }
 *
 * MyClass::modifyFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Item modified successfully";
 * }
 *
 * @endcode
 *
 * <h3>Conflict Resolution</h3>

 * When the job is executed, a check is made to ensure that the Item contained
 * in the job is not older than the version of the Item already held in the
 * Akonadi database. If it is older, a conflict resolution dialog is displayed
 * for the user to choose which version of the Item to use, unless
 * disableAutomaticConflictHandling() has been called to disable the dialog, or
 * disableRevisionCheck() has been called to disable version checking
 * altogether.
 *
 * The item version is checked by comparing the Item::revision() values in the
 * job and in the database. To ensure that two successive ItemModifyJobs for
 * the same Item work correctly, the revision number of the Item supplied to
 * the second ItemModifyJob should be set equal to the Item's revision number
 * on completion of the first ItemModifyJob. This can be obtained by, for
 * example, calling item().revision() in the job's result slot.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ItemModifyJob : public Job
{
    friend class ResourceBase;

    Q_OBJECT

public:
    /**
     * Creates a new item modify job.
     *
     * @param item The modified item object to store.
     * @param parent The parent object.
     */
    explicit ItemModifyJob(const Item &item, QObject *parent = 0);

    /**
     * Creates a new item modify job for bulk modifications.
     *
     * Using this is different from running a modification job per item.
     * Use this when applying the same change to a set of items, such as a
     * mass-change of item flags, not if you just want to store a bunch of
     * randomly modified items.
     *
     * Currently the following modifications are supported:
     * - flag changes
     *
     * @note Since this does not do payload modifications, it implies
     *       setIgnorePayload( true ) and disableRevisionCheck().
     * @param items The list of items to modify, must not be empty.
     * @since 4.6
     */
    explicit ItemModifyJob(const Item::List &items, QObject *parent = 0);

    /**
     * Destroys the item modify job.
     */
    virtual ~ItemModifyJob();

    /**
     * Sets whether the payload of the modified item shall be
     * omitted from transmission to the Akonadi storage.
     * The default is @c false, however it can be set for
     * performance reasons.
     * @param ignore ignores payload if set as @c true
     */
    void setIgnorePayload(bool ignore);

    /**
     * Returns whether the payload of the modified item shall be
     * omitted from transmission to the Akonadi storage.
     */
    bool ignorePayload() const;

    /**
     * Sets whether the GID shall be updated either from the gid parameter or
     * by extracting it from the payload.
     * The default is @c false to avoid unecessarily update the GID,
     * as it should never change once set, and the ItemCreateJob already sets it.
     * @param update update the GID if set as @c true
     *
     * @note If disabled the GID will not be updated, but still be used for identification of the item.
     * @since 4.12
     */
    void setUpdateGid(bool update);

    /**
     * Returns wheter the GID should be updated.
     * @since 4.12
     */
    bool updateGid() const;

    /**
     * Disables the check of the revision number.
     *
     * @note If disabled, no conflict detection is available.
     */
    void disableRevisionCheck();

    /**
     * Returns the modified and stored item including the changed revision number.
     *
     * @note Use this method only when using the single item constructor.
     */
    Item item() const;

    /**
     * Returns the modified and stored items including the changed revision number.
     *
     * @since 4.6
     */
    Item::List items() const;

    /**
     * Disables the automatic handling of conflicts.
     *
     * By default the item modify job will bring up a dialog to resolve
     * a conflict that might happen when modifying an item.
     * Calling this method will avoid that and the job returns with an
     * error in case of a conflict.
     *
     * @since 4.6
     */
    void disableAutomaticConflictHandling();

protected:
    void doStart() Q_DECL_OVERRIDE;
    void doHandleResponse(const QByteArray &tag, const QByteArray &data) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(ItemModifyJob)

    Q_PRIVATE_SLOT(d_func(), void conflictResolved())
    Q_PRIVATE_SLOT(d_func(), void conflictResolveError(const QString &))
    //@endcond
};

}

#endif
