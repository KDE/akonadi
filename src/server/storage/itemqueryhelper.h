/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ITEMQUERYHELPER_H
#define AKONADI_ITEMQUERYHELPER_H

#include "entities.h"

namespace Akonadi
{
class ImapSet;
class Scope;

namespace Server
{
class CommandContext;
class QueryBuilder;

/**
  Helper methods to generate WHERE clauses for item queries based on the item set
  used in the protocol.
*/
namespace ItemQueryHelper
{
/**
  Add conditions to @p qb for the given item set @p set. If @p collection is valid,
  only items in this collection are considered.
*/
void itemSetToQuery(const ImapSet &set, QueryBuilder &qb, const Collection &collection = Collection());

/**
  Convenience method, does essentially the same as the one above.
*/
void itemSetToQuery(const ImapSet &set, const CommandContext &context, QueryBuilder &qb);

/**
  Add conditions to @p qb for the given remote identifier @p rid.
  The rid context is taken from @p context.
*/
void remoteIdToQuery(const QStringList &rids, const CommandContext &context, QueryBuilder &qb);
void gidToQuery(const QStringList &gids, const CommandContext &context, QueryBuilder &qb);

/**
  Add conditions to @p qb for the given item operation scope @p scope.
  The rid context is taken from @p context, if none is specified an exception is thrown.
*/
void scopeToQuery(const Scope &scope, const CommandContext &context, QueryBuilder &qb);
}

} // namespace Server
} // namespace Akonadi

#endif
