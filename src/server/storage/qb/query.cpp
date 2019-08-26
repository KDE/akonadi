/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>

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

#include "query.h"
#include "akonadiserver_debug.h"
#ifndef QUERYBUILDER_UNITTEST
#include "../querycache.h"
#include "../datastore.h"
#include "../storagedebugger.h"
#include "../dbdeadlockcatcher.h"

#include <shared/akoptional.h>
#include <shared/akranges.h>

#include <QTextStream>
#include <QSqlError>

#include <functional>
#endif

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace Akonadi::Server::Qb;


Query::Query(DataStore &db)
#ifndef QUERYBUILDER_UNITTEST
    : mDb(db)
    , mQuery(db.database())
    , mDatabaseType(DbType::type(db.database()))
#endif
{
    Q_UNUSED(db);
}

void Query::setDatabaseType(DbType::Type type)
{
    mDatabaseType = type;
}

DbType::Type Query::databaseType() const
{
    return mDatabaseType;
}

QSqlQuery &Query::query()
{
    return mQuery;
}

QSqlError Query::error() const
{
    return mQuery.lastError();
}

int Query::size() const
{
    return mQuery.size();
}

void Query::finish()
{
    return mQuery.finish();
}

QueryResultIterator Query::begin() const
{
    return QueryResultIterator(mQuery);
}

QueryResultIterator Query::end() const
{
    return QueryResultIterator(QueryResultIterator::End(true));
}

#ifdef QUERYBUILDER_UNITTEST
bool Query::exec()
{
    return false;
}

#else

namespace
{

akOptional<QSqlQuery> prepareQuery(const QSqlDatabase &db, const QString &statement)
{
    auto query = QueryCache::query(statement);
    if (query.has_value()) {
        return query;
    } else {
        QSqlQuery query(db);
        if (!query.prepare(statement)) {
            qCCritical(AKONADISERVER_LOG) << "DATABASE ERROR while PREPARING QUERY:";
            qCCritical(AKONADISERVER_LOG) << "  Error code:" << query.lastError().nativeErrorCode();
            qCCritical(AKONADISERVER_LOG) << "  DB error: " << query.lastError().databaseText();
            qCCritical(AKONADISERVER_LOG) << "  Error text:" << query.lastError().text();
            qCCritical(AKONADISERVER_LOG) << "  Query:" << statement;
            return nullopt;
        }
        QueryCache::insert(statement, query);
        return query;
    }
}

bool doExecute(QSqlQuery &query, bool isBatch)
{
    if (isBatch) {
        return query.execBatch();
    } else {
        return query.exec();
    }
}

bool executeQuery(DataStore &store, QSqlQuery &query, bool isBatch)
{
    auto &storageDebugger = *StorageDebugger::instance();
    if (storageDebugger.isSQLDebuggingEnabled()) {
        QTime t;
        t.start();
        if (!doExecute(query, isBatch)) {
            return false;
        }
        storageDebugger.queryExecuted(reinterpret_cast<qint64>(&store), query, t.elapsed());
        return true;
    } else {
        storageDebugger.incSequence();
        return doExecute(query, isBatch);
    }
}

} // namespace

bool Query::exec()
{
    QString statement;
    QTextStream stream(&statement);
    serialize(stream);

    auto query = prepareQuery(mDb.database(), statement);
    if (!query) {
        return false;
    }
    mQuery = std::move(*query);

    // Bind values to query
    const auto boundValues = bindValues();
    boundValues | forEach(std::bind(&QSqlQuery::addBindValue, mQuery, std::placeholders::_1, QSql::In));
    const auto isBatch = boundValues | any([](const auto &val) { return static_cast<QMetaType::Type>(val.type()) == QMetaType::QVariantList; });

    if (executeQuery(mDb, mQuery, isBatch)) {
        return true;
    }

    bool needsRetry = false;
    // Handle transaction deadlocks and timeouts by attempting to replay the transaction.
    if (mDatabaseType == DbType::PostgreSQL) {
        const QString dbError = mQuery.lastError().databaseText();
        if (dbError.contains(QLatin1String("40P01" /* deadlock_detected */))) {
            qCWarning(AKONADISERVER_LOG) << "QueryBuilder::exec(): database reported transaction deadlock, retrying transaction";
            qCWarning(AKONADISERVER_LOG) << mQuery.lastError().text();
            needsRetry = true;
        }
    } else if (mDatabaseType == DbType::MySQL) {
        const QString lastErrorStr = mQuery.lastError().nativeErrorCode();
        const int error = lastErrorStr.isEmpty() ? -1 : lastErrorStr.toInt();
        if (error == 1213 /* ER_LOCK_DEADLOCK */) {
            qCWarning(AKONADISERVER_LOG) << "QueryBuilder::exec(): database reported transaction deadlock, retrying transaction";
            qCWarning(AKONADISERVER_LOG) << mQuery.lastError().text();
            needsRetry = true;
        } else if (error == 1205 /* ER_LOCK_WAIT_TIMEOUT */) {
            qCWarning(AKONADISERVER_LOG) << "QueryBuilder::exec(): database reported transaction timeout, retrying transaction";
            qCWarning(AKONADISERVER_LOG) << mQuery.lastError().text();
            // Not sure retrying helps, maybe error is good enough.... but doesn't hurt to retry a few times before giving up.
            needsRetry = true;
        }
    } else if (mDatabaseType == DbType::Sqlite && !DbType::isSystemSQLite(DataStore::self()->database())) {
        const QString lastErrorStr = mQuery.lastError().nativeErrorCode();
        const int error = lastErrorStr.isEmpty() ? -1 : lastErrorStr.toInt();
        if (error == 6 /* SQLITE_LOCKED */) {
            qCWarning(AKONADISERVER_LOG) << "QueryBuilder::exec(): database reported transaction deadlock, retrying transaction";
            qCWarning(AKONADISERVER_LOG) << mQuery.lastError().text();
            DataStore::self()->doRollback();
            needsRetry = true;
        } else if (error == 5 /* SQLITE_BUSY */) {
            qCWarning(AKONADISERVER_LOG) << "QueryBuilder::exec(): database reported transaction timeout, retrying transaction";
            qCWarning(AKONADISERVER_LOG) << mQuery.lastError().text();
            DataStore::self()->doRollback();
            needsRetry = true;
        }
    } else if (mDatabaseType == DbType::Sqlite) {
        // We can't have a transaction deadlock in SQLite when using driver shipped
        // with Qt, because it does not support concurrent transactions and DataStore
        // serializes them through a global lock.
    }

    if (needsRetry) {
        DataStore::self()->transactionKilledByDB();
        throw DbDeadlockException(mQuery);
    }

    qCCritical(AKONADISERVER_LOG) << "DATABASE ERROR:";
    qCCritical(AKONADISERVER_LOG) << "  Error code:" << mQuery.lastError().nativeErrorCode();
    qCCritical(AKONADISERVER_LOG) << "  DB error: " << mQuery.lastError().databaseText();
    qCCritical(AKONADISERVER_LOG) << "  Error text:" << mQuery.lastError().text();
    qCCritical(AKONADISERVER_LOG) << "  Values:" << mQuery.boundValues();
    qCCritical(AKONADISERVER_LOG) << "  Query:" << statement;
    return false;
}

#endif // QUERYBUILDER_UNITTEST

