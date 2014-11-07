/*
    Copyright (c) 2007-2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ITEMMONITOR_H
#define AKONADI_ITEMMONITOR_H

#include "akonadicore_export.h"
#include <qglobal.h>

namespace Akonadi {

class Item;
class ItemFetchScope;

/**
 * @short A convenience class to monitor a single item for changes.
 *
 * This class can be used as a base class for classes that want to show
 * a single item to the user and keep track of status changes of the item
 * without having to using a Monitor object themself.
 *
 * Example:
 *
 * @code
 *
 * // A label that shows the name of a contact item
 *
 * class ContactLabel : public QLabel, public Akonadi::ItemMonitor
 * {
 *   public:
 *     ContactLabel( QWidget *parent = 0 )
 *       : QLabel( parent )
 *     {
 *       setText( "No Name" );
 *     }
 *
 *   protected:
 *     virtual void itemChanged( const Akonadi::Item &item )
 *     {
 *       if ( item.mimeType() != "text/directory" )
 *         return;
 *
 *       const KContacts::Addressee addr = item.payload<KContacts::Addressee>();
 *       setText( addr.fullName() );
 *     }
 *
 *     virtual void itemRemoved()
 *     {
 *       setText( "No Name" );
 *     }
 * };
 *
 * ...
 *
 * ContactLabel *label = new ContactLabel( this );
 *
 * const Akonadi::Item item = fetchJob->items().first();
 * label->setItem( item );
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT ItemMonitor
{
public:
    /**
     * Creates a new item monitor.
     */
    ItemMonitor();

    /**
     * Destroys the item monitor.
     */
    virtual ~ItemMonitor();

    /**
     * Sets the @p item that shall be monitored.
     */
    void setItem(const Item &item);

    /**
     * Returns the currently monitored item.
     */
    Item item() const;

protected:
    /**
     * This method is called whenever the monitored item has changed.
     *
     * @param item The changed item.
     */
    virtual void itemChanged(const Item &item);

    /**
     * This method is called whenever the monitored item has been removed.
     */
    virtual void itemRemoved();

    /**
     * Sets the item fetch scope.
     *
     * Controls how much of an item's data is fetched from the server, e.g.
     * whether to fetch the full item payload or only meta data.
     *
     * @param fetchScope The new scope for item fetch operations.
     *
     * @see fetchScope()
     */
    void setFetchScope(const ItemFetchScope &fetchScope);

    /**
     * Returns the item fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the ItemFetchScope documentation
     * for an example.
     *
     * @return a reference to the current item fetch scope
     *
     * @see setFetchScope() for replacing the current item fetch scope
     */
    ItemFetchScope &fetchScope();

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond

    Q_DISABLE_COPY(ItemMonitor)
};

}

#endif
