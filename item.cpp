/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "item.h"

using namespace Akonadi;

class Akonadi::ItemPrivate {
  public:
    DataReference ref;
    Item::Flags flags;
    QByteArray data;
    QByteArray mimeType;
};

Item::Item( const DataReference & ref ) :
    d( new ItemPrivate )
{
  d->ref = ref;
}

Item::~ Item( )
{
  delete d;
}

DataReference Item::reference( ) const
{
  return d->ref;
}

Item::Flags Item::flags( ) const
{
  return d->flags;
}

void Item::setFlag( const QByteArray & name )
{
  d->flags.insert( name );
}

void Item::unsetFlag( const QByteArray & name )
{
  d->flags.remove( name );
}

bool Item::hasFlag( const QByteArray & name ) const
{
  return d->flags.contains( name );
}

QByteArray Item::data() const
{
  return d->data;
}

void Item::setData(const QByteArray & data)
{
  d->data = data;
}

QByteArray Item::mimeType() const
{
  return d->mimeType;
}

void Item::setMimeType(const QByteArray & mimeType)
{
  d->mimeType = mimeType;
}
