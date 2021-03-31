/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <optional>

class QString;
class QSqlQuery;

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
/// Returns the cached (and prepared) query for @p queryStatement
std::optional<QSqlQuery> query(const QString &queryStatement);

/// Insert @p query into the cache for @p queryStatement.
void insert(const QString &queryStatement, const QSqlQuery &query);

/// Clears all queries from current thread
void clear();

} // namespace QueryCache

} // namespace Server
} // namespace Akonadi

