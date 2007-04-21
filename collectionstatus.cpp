/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collectionstatus.h"

#include <QSharedData>

using namespace Akonadi;

class CollectionStatus::Private : public QSharedData
{
  public:
    Private() :
      QSharedData(),
      count( -1 ),
      unreadCount( -1 )
    {}

    Private( const Private &other ) :
      QSharedData( other )
    {
      count = other.count;
      unreadCount = other.count;
    }

    int count;
    int unreadCount;
};


CollectionStatus::CollectionStatus() :
    d( new Private )
{
}

CollectionStatus::CollectionStatus(const CollectionStatus &other) :
    d( other.d )
{
}

CollectionStatus::~CollectionStatus()
{
}

int CollectionStatus::count( ) const
{
  return d->count;
}

void CollectionStatus::setCount( int count )
{
  d->count = count;
}

int CollectionStatus::unreadCount( ) const
{
  return d->unreadCount;
}

void CollectionStatus::setUnreadCount( int count )
{
  d->unreadCount = count;
}

CollectionStatus& CollectionStatus::operator =(const CollectionStatus & other)
{
  d = other.d;
  return *this;
}
