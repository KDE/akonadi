/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONQUERYHELPER_H
#define AKONADI_COLLECTIONQUERYHELPER_H

#include "entities.h"

#include <private/scope_p.h>

namespace Akonadi
{

namespace Server
{

class Connection;
class QueryBuilder;

/**
  Helper methods to generate WHERE clauses for collection queries based on a Scope object.
*/
namespace CollectionQueryHelper
{

/**
  Add conditions to @p qb for the given remote identifier @p rid.
  The rid context is taken from @p connection.
*/
void remoteIdToQuery(const QStringList &rids, Connection *connection, QueryBuilder &qb);

/**
  Add conditions to @p qb for the given collection operation scope @p scope.
  The rid context is taken from @p connection, if none is specified an exception is thrown.
*/
void scopeToQuery(const Scope &scope, Connection *connection, QueryBuilder &qb);

/**
  Checks if a collection could exist in the given parent folder with the given name.
*/
bool hasAllowedName(const Collection &collection, const QString &name, Collection::Id parent);

/**
  Checks if a collection could be moved from its current parent into the given one.
*/
bool canBeMovedTo(const Collection &collection, const Collection &parent);

/**
  Retrieve the collection referred to by the given hierarchical RID chain.
*/
Collection resolveHierarchicalRID(const QVector<Scope::HRID> &hridChain, Resource::Id resId);

/**
  Returns an existing collection specified by the given scope. If that does not
  specify exactly one valid collection, an exception is thrown.
*/
Collection singleCollectionFromScope(const Scope &scope, Connection *connection);
}

} // namespace Server
} // namespace Akonadi

#endif
