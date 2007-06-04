/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ITEMBROWSER_H
#define AKONADI_ITEMBROWSER_H

#include "libakonadi_export.h"
#include <libakonadi/itemview.h>
#include <ktextbrowser.h>

namespace Akonadi {

class Item;

/**
 * This class is a base class for item view browsers.
 *
 * Item view browsers are widgets that show information about one item
 * of the Akonadi storage as richtext in a KTextBrowser.
 *
 * @see KABCItemBrowser
 * @see KCalItemBrowser
 */
class AKONADI_EXPORT ItemBrowser : public KTextBrowser, public ItemView
{
  public:
    /**
     * Creates a new item browser.
     */
    ItemBrowser( QWidget *parent = 0 );

    /**
     * Destroys the item browser.
     */
    virtual ~ItemBrowser();

  protected:
    /**
     * This method is called whenever the item was added or has changed.
     *
     * You should reimplement it and return a richtext representation of
     * the passed item.
     */
    virtual QString itemToRichText( const Item &item );

  private:
    void itemAdded( const Item &item );
    void itemChanged( const Item &item );
    void itemRemoved();

    class Private;
    Private* const d;

    Q_DISABLE_COPY( ItemBrowser )
};

}

#endif
