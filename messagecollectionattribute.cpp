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

#include "messagecollectionattribute.h"

using namespace Akonadi;

class MessageCollectionAttribute::Private
{
  public:
    int count;
    int unreadCount;
};


MessageCollectionAttribute::MessageCollectionAttribute() :
    CollectionAttribute(),
    d( new Private() )
{
  d->count = -1;
  d->unreadCount = -1;
}

MessageCollectionAttribute::~MessageCollectionAttribute()
{
  delete d;
}

int MessageCollectionAttribute::count( ) const
{
  return d->count;
}

void MessageCollectionAttribute::setCount( int count )
{
  d->count = count;
}

int MessageCollectionAttribute::unreadCount( ) const
{
  return d->unreadCount;
}

void MessageCollectionAttribute::setUnreadCount( int count )
{
  d->unreadCount = count;
}

QByteArray MessageCollectionAttribute::type( ) const
{
  return "MessageCollection";
}

MessageCollectionAttribute * MessageCollectionAttribute::clone() const
{
  MessageCollectionAttribute* attr =  new MessageCollectionAttribute();
  attr->setCount( count() );
  attr->setUnreadCount( unreadCount() );
  return attr;
}

QByteArray MessageCollectionAttribute::toByteArray() const
{
  // TODO
  return QByteArray();
}

void MessageCollectionAttribute::setData(const QByteArray & data)
{
  // TODO
}
