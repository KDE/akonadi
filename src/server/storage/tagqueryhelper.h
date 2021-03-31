/*
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "entities.h"

namespace Akonadi
{
class Scope;

namespace Server
{
class CommandContext;
class QueryBuilder;

/**
  Helper methods to generate WHERE clauses for item queries based on the item set
  used in the protocol.
*/
namespace TagQueryHelper
{
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

