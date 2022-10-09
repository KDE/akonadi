/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "entities.h"

#include <private/scope_p.h>

namespace Akonadi
{
namespace Server
{
class CommandContext;
class QueryBuilder;

/**
  Helper methods to generate WHERE clauses for collection queries based on a Scope object.
*/
namespace CollectionQueryHelper
{
/**
  Add conditions to @p qb for the given remote identifier @p rid.
  The rid context is taken from @p context.
*/
void remoteIdToQuery(const QStringList &rids, const CommandContext &context, QueryBuilder &qb);

/**
  Add conditions to @p qb for the given collection operation scope @p scope.
  The rid context is taken from @p context, if none is specified an exception is thrown.
*/
void scopeToQuery(const Scope &scope, const CommandContext &context, QueryBuilder &qb);

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
Collection singleCollectionFromScope(const Scope &scope, const CommandContext &context);
}

} // namespace Server
} // namespace Akonadi
