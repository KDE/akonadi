/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "itemfetchscope.h"

#include "itemfetchscope_p.h"

#include <QtCore/QStringList>

using namespace Akonadi;

ItemFetchScope::ItemFetchScope()
{
  d = new ItemFetchScopePrivate();
}

ItemFetchScope::ItemFetchScope( const ItemFetchScope &other )
  : d(other.d)
{
}

ItemFetchScope::~ItemFetchScope()
{
}

ItemFetchScope &ItemFetchScope::operator=( const ItemFetchScope &other )
{
  if ( &other == this )
    return *this;

  d = other.d;
  return *this;
}

void ItemFetchScope::addFetchPart( const QString &identifier )
{
  d->mFetchParts.insert( identifier );
}

QStringList ItemFetchScope::fetchPartList() const
{
    return d->mFetchParts.toList();
}

void ItemFetchScope::setFetchAllParts( bool fetchAll )
{
  d->mFetchAllParts = fetchAll;
}

bool ItemFetchScope::fetchAllParts() const
{
  return d->mFetchAllParts;
}
