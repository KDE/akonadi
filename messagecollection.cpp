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

#include "messagecollection.h"

using namespace PIM;

class PIM::MessageCollection::Private
{
  public:
    int count;
    int unreadCount;
};


PIM::MessageCollection::MessageCollection( const QByteArray &path ) :
    Collection( path ),
    d( new Private() )
{
  d->count = -1;
  d->unreadCount = -1;
}

PIM::MessageCollection::MessageCollection( const MessageCollection & other ) :
    Collection( other ),
    d( new Private() )
{
  d->count = other.count();
  d->unreadCount = other.unreadCount();
}

PIM::MessageCollection::~MessageCollection()
{
  delete d;
  d = 0;
}

int PIM::MessageCollection::count( ) const
{
  return d->count;
}

void PIM::MessageCollection::setCount( int count )
{
  d->count = count;
}

int PIM::MessageCollection::unreadCount( ) const
{
  return d->unreadCount;
}

void PIM::MessageCollection::setUnreadCount( int count )
{
  d->unreadCount = count;
}
