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

#include "itembrowser.h"

using namespace Akonadi;

#include <libakonadi/item.h>

ItemBrowser::ItemBrowser( QWidget *parent )
  : KTextBrowser( parent ), d( 0 )
{
}

ItemBrowser::~ItemBrowser()
{
  // delete d;
}

void ItemBrowser::itemAdded( const Item &item )
{
  setHtml( itemToRichText( item ) );
}

void ItemBrowser::itemChanged( const Item &item )
{
  setHtml( itemToRichText( item ) );
}

void ItemBrowser::itemRemoved()
{
  setHtml( QLatin1String( "<html><body><center>The watched item has been deleted</center></body></html>" ) );
}

QString ItemBrowser::itemToRichText( const Item &item )
{
  return QString::fromUtf8( item.part( QLatin1String( "RFC822" ) ) );
}
