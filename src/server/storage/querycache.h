/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <optional>

class QString;
class QSqlQuery;
class QSqlDatabase;

namespace Akonadi
{
namespace Server
{
/**
 * A per-thread cache (should be per session, but that'S the same for us) prepared
 * query cache.
 */
namespace QueryCache
{

/**
 * Return a cached QSqlQuery for given @p queryStatement.
 *
 * If no query is cached for @p queryStatement, an empty optional is returned. Otherwise
 * the cached query is removed from the cache and must be returned with insert()
 * after use.
 */
std::optional<QSqlQuery> query(const QString &queryStatement);

/// Insert @p query into the cache for @p queryStatement.
void insert(const QSqlDatabase &db, const QString &queryStatement, QSqlQuery query);

/// Clears all queries from current thread
void clear();

/// Returns the per-thread capacityof the query cache
size_t capacity();

} // namespace QueryCache

} // namespace Server
} // namespace Akonadi
