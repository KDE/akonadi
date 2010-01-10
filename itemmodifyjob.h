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

#include "akonadi_export.h"

#include <akonadi/item.h>
#include <akonadi/job.h>

namespace Akonadi {

class Collection;
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
 * connect( fetchJob, SIGNAL( result( KJob* ) ), SLOT( fetchFinished( KJob* ) ) );
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
 *   connect( modifyJob, SIGNAL( result( KJob* ) ), SLOT( modifyFinished( KJob* ) ) );
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
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT ItemModifyJob : public Job
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
    explicit ItemModifyJob( const Item &item, QObject *parent = 0 );

    /**
     * Destroys the item modify job.
     */
    virtual ~ItemModifyJob();

    /**
     * Sets whether the payload of the modified item shall be
     * omitted from transmission to the Akonadi storage.
     * The default is @c false, however it can be set for
     * performance reasons.
     */
    void setIgnorePayload( bool ignore );

    /**
     * Returns whether the payload of the modified item shall be
     * omitted from transmission to the Akonadi storage.
     */
    bool ignorePayload() const;

    /**
     * Disables the check of the revision number.
     *
     * @note If disabled, no conflict detection is available.
     */
    void disableRevisionCheck();

    /**
     * Returns the modified and stored item including the changed revision number.
     */
    Item item() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( ItemModifyJob )
};

}

#endif
