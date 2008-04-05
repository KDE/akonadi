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

#include "akonadi_export.h"

class QStringList;

namespace Akonadi {

class Item;
class ItemFetchScope;

/**
 * The class ItemMonitor is a convenience class to monitor a single item.
 *
 * This class can be used as a base class for classes that want to show
 * a single item to the user and keep track of status changes of the item
 * without having to using a Monitor object themself.
 */
class AKONADI_EXPORT ItemMonitor
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
     * Sets the item that shall be monitored.
     */
    void setItem( const Item &id );

    /**
     * Returns the currently monitored item.
     */
     Item item() const;

  protected:
    /**
     * This method is called whenever the monitored item has changed.
     *
     * @param item The data of the changed item.
     */
    virtual void itemChanged( const Item &item );

    /**
     * This method is called whenever the monitored item has been removed.
     */
    virtual void itemRemoved();

    /**
      Sets the item fetch scope.

      Controls how much of an item's data is fetched from the server, e.g.
      whether to fetch the full item payload or only metadata.

      @param fetchScope the new scope for item fetch operations

      @see fetchScope()
    */
    void setFetchScope( const ItemFetchScope &fetchScope );

    /**
      Returns the item fetch scope.

      Since this returns a reference it can be used to conveniently modify the
      current scope in-place, i.e. by calling a method on the returned reference
      without storing it in a local variable. See the ItemFetchScope documentation
      for an example.

      @return a reference to the current item fetch scope

      @see setFetchScope() for replacing the current item fetch scope
    */
    ItemFetchScope &fetchScope();

  private:
    class Private;
    Private* const d;

    Q_DISABLE_COPY( ItemMonitor )
};

}

#endif
