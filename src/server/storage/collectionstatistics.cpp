/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 * Copyright (C) 2016  Daniel Vrátil <dvratil@kde.org>
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
#include "datastore.h"

#include <private/protocol_p.h>


using namespace Akonadi::Server;

CollectionStatistics *CollectionStatistics::sInstance = nullptr;

CollectionStatistics *CollectionStatistics::self()
{
    if (sInstance == nullptr) {
        sInstance = new CollectionStatistics();
    }
    return sInstance;
}

void CollectionStatistics::destroy()
{
    delete sInstance;
    sInstance = nullptr;
}

CollectionStatistics::CollectionStatistics(bool prefetch)
{
    if (prefetch) {
        QMutexLocker lock(&mCacheLock);

        QList<QueryBuilder> builders;
        // This single query will give us statistics for all non-empty non-virtual
        // Collections at much better speed than individual queries.
        auto qb = prepareGenericQuery();
        qb.addColumn(PimItem::collectionIdFullColumnName());
        qb.addGroupColumn(PimItem::collectionIdFullColumnName());
        builders << qb;

        // This single query will give us statistics for all non-empty virtual
        // Collections
        qb = prepareGenericQuery();
        qb.addColumn(CollectionPimItemRelation::leftFullColumnName());
        qb.addJoin(QueryBuilder::InnerJoin, CollectionPimItemRelation::tableName(),
                   CollectionPimItemRelation::rightFullColumnName(), PimItem::idFullColumnName());
        qb.addGroupColumn(CollectionPimItemRelation::leftFullColumnName());
        builders << qb;

        for (auto &qb : builders) {
            if (!qb.exec()) {
                return;
            }

            auto query = qb.query();
            while (query.next()) {
                mCache.insert(query.value(3).toLongLong(),
                            { query.value(0).toLongLong(),
                                query.value(1).toLongLong(),
                                query.value(2).toLongLong()
                            });
            }
        }

        // Now quickly get all non-virtual enabled Collections and if they are
        // not in mCache yet, insert them with empty statistics.
        qb = QueryBuilder(Collection::tableName());
        qb.addColumn(Collection::idColumn());
        qb.addValueCondition(Collection::enabledColumn(), Query::Equals, true);
        qb.addValueCondition(Collection::isVirtualColumn(), Query::Equals, false);
        if (!qb.exec()) {
            return;
        }

        auto query = qb.query();
        while (query.next()) {
            const auto colId = query.value(0).toLongLong();
            if (!mCache.contains(colId)) {
                mCache.insert(colId, { 0, 0, 0 });
            }
        }
    }
}

void CollectionStatistics::itemAdded(const Collection &col, qint64 size, bool seen)
{
    if (!col.isValid()) {
        return;
    }

    QMutexLocker lock(&mCacheLock);
    auto stats = mCache.find(col.id());
    if (stats != mCache.end()) {
        ++(stats->count);
        stats->size += size;
        stats->read += (seen ? 1 : 0);
    } else {
        mCache.insert(col.id(), calculateCollectionStatistics(col));
    }
}

void CollectionStatistics::itemsSeenChanged(const Collection &col, qint64 seenCount)
{
    if (!col.isValid()) {
        return;
    }

    QMutexLocker lock(&mCacheLock);
    auto stats = mCache.find(col.id());
    if (stats != mCache.end()) {
        stats->read += seenCount;
    } else {
        mCache.insert(col.id(), calculateCollectionStatistics(col));
    }
}

void CollectionStatistics::invalidateCollection(const Collection &col)
{
    if (!col.isValid()) {
        return;
    }

    QMutexLocker lock(&mCacheLock);
    mCache.remove(col.id());
}

void CollectionStatistics::expireCache()
{
    QMutexLocker lock(&mCacheLock);
    mCache.clear();
}

const CollectionStatistics::Statistics CollectionStatistics::statistics(const Collection &col)
{
    QMutexLocker lock(&mCacheLock);
    auto it = mCache.find(col.id());
    if (it == mCache.end()) {
        it = mCache.insert(col.id(), calculateCollectionStatistics(col));
    }
    return it.value();
}

QueryBuilder CollectionStatistics::prepareGenericQuery()
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
                                        Flag::retrieveByNameOrCreate(QStringLiteral(AKONADI_FLAG_SEEN)).id());
        qb.addJoin(QueryBuilder::LeftJoin,
                   QStringLiteral("%1 AS %2").arg(PimItemFlagRelation::tableName(), SeenFlagsTableName),
                   seenCondition);
    }
    {
        Query::Condition ignoredCondition(Query::And);
        ignoredCondition.addColumnCondition(PimItem::idFullColumnName(), Query::Equals, FLAGS_COLUMN(IgnoredFlags, leftColumn));
        ignoredCondition.addValueCondition(FLAGS_COLUMN(IgnoredFlags, rightColumn), Query::Equals,
                                           Flag::retrieveByNameOrCreate(QStringLiteral(AKONADI_FLAG_IGNORED)).id());
        qb.addJoin(QueryBuilder::LeftJoin,
                   QStringLiteral("%1 AS %2").arg(PimItemFlagRelation::tableName(), IgnoredFlagsTableName),
                   ignoredCondition);
    }

    #undef FLAGS_COLUMN

    return qb;
}

CollectionStatistics::Statistics CollectionStatistics::calculateCollectionStatistics(const Collection &col)
{
    auto qb = prepareGenericQuery();

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

    auto result = Statistics{ qb.query().value(0).toLongLong(),
                              qb.query().value(1).toLongLong(),
                              qb.query().value(2).toLongLong() };
    qb.query().finish();
    return result;
}
