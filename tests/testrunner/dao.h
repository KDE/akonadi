/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DAO_H
#define DAO_H

#include <akonadi/collection.h>
#include <akonadi/item.h>

class DAO
{
  public:
    bool insertItem( const Akonadi::Item &item, const Akonadi::Collection &collection );
    Akonadi::Collection::List collections() const;
    Akonadi::Collection collectionByName( const QString &collectionName ) const;
};

#endif
