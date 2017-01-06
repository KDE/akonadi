/*
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_QUERYCACHE_H
#define AKONADI_QUERYCACHE_H

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
/// Check whether the query @p queryStatement is cached already.
bool contains(const QString &queryStatement);

/// Returns the cached (and prepared) query for @p queryStatement.
QSqlQuery query(const QString &queryStatement);

/// Insert @p query into the cache for @p queryStatement.
void insert(const QString &queryStatement, const QSqlQuery &query);

/// Clears all queries from current thread
void clear();

} // namespace QueryCache

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_QUERYCACHE_H
