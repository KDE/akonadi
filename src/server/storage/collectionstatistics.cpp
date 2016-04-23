/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "collectionstatistics.h"
#include "querybuilder.h"
#include "countquerybuilder.h"
#include "akonadiserver_debug.h"
#include "entities.h"

#include <private/protocol_p.h>

#include <QDateTime>

using namespace Akonadi::Server;

CollectionStatistics *CollectionStatistics::sInstance = 0;

CollectionStatistics *CollectionStatistics::self()
{
    if (sInstance == 0) {
        sInstance = new CollectionStatistics();
    }
    return sInstance;
}

void CollectionStatistics::invalidateCollection(const Collection &col)
{
    QMutexLocker lock(&mCacheLock);
    mCache.remove(col.id());
}

const CollectionStatistics::Statistics &CollectionStatistics::statistics(const Collection &col)
{
    QMutexLocker lock(&mCacheLock);
    auto it = mCache.find(col.id());
    if (it == mCache.end()) {
        it = mCache.insert(col.id(), getCollectionStatistics(col));
    }
    return it.value();
}

CollectionStatistics::Statistics CollectionStatistics::getCollectionStatistics(const Collection &col)
{
    static const QString SeenFlagsTableName = QStringLiteral("SeenFlags");
    static const QString IgnoredFlagsTableName = QStringLiteral("IgnoredFlags");

#define FLAGS_COLUMN(table, column) \
    QStringLiteral("%1.%2").arg(table##TableName, PimItemFlagRelation::column())

    // COUNT(DISTINCT PimItemTable.id)
    CountQueryBuilder qb(PimItem::tableName(), PimItem::idFullColumnName(), CountQueryBuilder::Distinct);
    // SUM(PimItemTable.size)
    qb.addAggregation(PimItem::sizeFullColumnName(), QStringLiteral("sum"));

    // SUM(CASE WHEN SeenFlags.flag_id IS NOT NULL OR IgnoredFlags.flag_id IS NOT NULL THEN 1 ELSE 0 END)
    // This allows us to get read messages count in a single query with the other
    // statistics. It is much than doing two queries, because the database
    // only has to calculate the JOINs once.
    //
    // Flag::retrieveByName() will hit the Entity cache, which allows us to avoid
    // a second JOIN with FlagTable, which PostgreSQL seems to struggle to optimize.
    Query::Condition cond(Query::Or);
    cond.addValueCondition(FLAGS_COLUMN(SeenFlags, rightColumn), Query::IsNot, QVariant());
    cond.addValueCondition(FLAGS_COLUMN(IgnoredFlags, rightColumn), Query::IsNot, QVariant());

    Query::Case caseStmt(cond, QStringLiteral("1"), QStringLiteral("0"));
    qb.addAggregation(caseStmt, QStringLiteral("sum"));

    // We need to join PimItemFlagRelation table twice - once for \SEEN flag and once
    // for $IGNORED flag, otherwise entries from PimItemTable get duplicated when an
    // item has both flags and the SUM(CASE ...) above returns bogus values
    {
        Query::Condition seenCondition(Query::And);
        seenCondition.addColumnCondition(PimItem::idFullColumnName(), Query::Equals, FLAGS_COLUMN(SeenFlags, leftColumn));
        seenCondition.addValueCondition(FLAGS_COLUMN(SeenFlags, rightColumn), Query::Equals,
                                        Flag::retrieveByName(QStringLiteral(AKONADI_FLAG_SEEN)).id());
        qb.addJoin(QueryBuilder::LeftJoin,
                   QStringLiteral("%1 AS %2").arg(PimItemFlagRelation::tableName(), SeenFlagsTableName),
                   seenCondition);
    }
    {
        Query::Condition ignoredCondition(Query::And);
        ignoredCondition.addColumnCondition(PimItem::idFullColumnName(), Query::Equals, FLAGS_COLUMN(IgnoredFlags, leftColumn));
        ignoredCondition.addValueCondition(FLAGS_COLUMN(IgnoredFlags, rightColumn), Query::Equals,
                                           Flag::retrieveByName(QStringLiteral(AKONADI_FLAG_IGNORED)).id());
        qb.addJoin(QueryBuilder::LeftJoin,
                   QStringLiteral("%1 AS %2").arg(PimItemFlagRelation::tableName(), IgnoredFlagsTableName),
                   ignoredCondition);
    }

#undef FLAGS_COLUMN

    if (col.isVirtual()) {
        qb.addJoin(QueryBuilder::InnerJoin, CollectionPimItemRelation::tableName(),
                   CollectionPimItemRelation::rightFullColumnName(), PimItem::idFullColumnName());
        qb.addValueCondition(CollectionPimItemRelation::leftFullColumnName(), Query::Equals, col.id());
    } else {
        qb.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, col.id());
    }

    if (!qb.exec()) {
        return { -1, -1, -1 };
    }
    if (!qb.query().next()) {
        qCCritical(AKONADISERVER_LOG) << "Error during retrieving result of statistics query:" << qb.query().lastError().text();
        return { -1, -1, -1 };
    }

    return { qb.query().value(0).toLongLong(),
             qb.query().value(1).toLongLong(),
             qb.query().value(2).toLongLong() };
}
