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
#include "akdebug.h"
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
    // COUNT(DISTINCT PimItemTable.id)
    CountQueryBuilder qb(PimItem::tableName(), PimItem::idFullColumnName(), CountQueryBuilder::Distinct);
    // SUM(PimItemTable.size)
    qb.addAggregation(PimItem::sizeFullColumnName(), QStringLiteral("sum"));
    // SUM(CASE WHEN FlagTable.name IN ('\SEEN', '$IGNORED') THEN 1 ELSE 0 END)
    // This allows us to get read messages count in a single query with the other
    // statistics. It is much than doing two queries, because the database
    // only has to calculate the JOINs once.
    //
    // Flag::retrieveByName() will hit the Entity cache, which allows us to avoid
    // a second JOIN with FlagTable, which PostgreSQL seems to struggle to optimize.
    Query::Condition cond(Query::Or);
    cond.addValueCondition(PimItemFlagRelation::rightFullColumnName(),
                           Query::Equals,
                           Flag::retrieveByName(QStringLiteral(AKONADI_FLAG_SEEN)).id());
    cond.addValueCondition(PimItemFlagRelation::rightFullColumnName(),
                           Query::Equals,
                           Flag::retrieveByName(QStringLiteral(AKONADI_FLAG_IGNORED)).id());
    Query::Case caseStmt(cond, QStringLiteral("1"), QStringLiteral("0"));
    qb.addAggregation(caseStmt, QStringLiteral("sum"));

    qb.addJoin(QueryBuilder::LeftJoin, PimItemFlagRelation::tableName(),
               PimItem::idFullColumnName(), PimItemFlagRelation::leftFullColumnName());
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
        akError() << "Error during retrieving result of statistics query:" << qb.query().lastError().text();
        return { -1, -1, -1 };
    }

    return { qb.query().value(0).toLongLong(),
             qb.query().value(1).toLongLong(),
             qb.query().value(2).toLongLong() };
}
