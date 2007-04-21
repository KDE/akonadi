/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SELECTQUERYBUILDER_H
#define AKONADI_SELECTQUERYBUILDER_H

#include "storage/datastore.h"
#include "storage/querybuilder.h"

namespace Akonadi {

/**
  Helper class for creating and executing database SELECT queries.
*/
template <typename T> class SelectQueryBuilder : public QueryBuilder
{
  public:
    /**
      Creates a new query builder.
    */
    inline SelectQueryBuilder() : QueryBuilder( Select )
    {
      addColumns( T::fullColumnNames() );
      addTable( T::tableName() );
    }

    /**
      Returns the result of this SELECT query.
    */
    QList<T> result()
    {
      return T::extractResult( query() );
    }
};

}

#endif
