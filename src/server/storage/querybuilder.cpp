/*
    SPDX-FileCopyrightText: 2007-2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "querybuilder.h"
#include "akonadiserver_debug.h"
#include "dbexception.h"
#include <qglobal.h>

#ifndef QUERYBUILDER_UNITTEST
#include "storage/datastore.h"
#include "storage/querycache.h"
#include "storage/storagedebugger.h"
#endif

#include "shared/akranges.h"

#include <QElapsedTimer>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlDriver>

using namespace Akonadi::Server;

namespace
{

DataStore *defaultDataStore()
{
#ifdef QUERYBUILDER_UNITTEST
    return nullptr;
#else
    return DataStore::self();
#endif
}

} // namespace

static QLatin1StringView compareOperatorToString(Query::CompareOperator op)
{
    switch (op) {
    case Query::Equals:
        return QLatin1StringView(" = ");
    case Query::NotEquals:
        return QLatin1StringView(" <> ");
    case Query::Is:
        return QLatin1StringView(" IS ");
    case Query::IsNot:
        return QLatin1StringView(" IS NOT ");
    case Query::Less:
        return QLatin1StringView(" < ");
    case Query::LessOrEqual:
        return QLatin1StringView(" <= ");
    case Query::Greater:
        return QLatin1StringView(" > ");
    case Query::GreaterOrEqual:
        return QLatin1StringView(" >= ");
    case Query::In:
        return QLatin1StringView(" IN ");
    case Query::NotIn:
        return QLatin1StringView(" NOT IN ");
    case Query::Like:
        return QLatin1StringView(" LIKE ");
    }
    Q_ASSERT_X(false, "QueryBuilder::compareOperatorToString()", "Unknown compare operator.");
    return QLatin1StringView("");
}

static QLatin1StringView logicOperatorToString(Query::LogicOperator op)
{
    switch (op) {
    case Query::And:
        return QLatin1StringView(" AND ");
    case Query::Or:
        return QLatin1StringView(" OR ");
    }
    Q_ASSERT_X(false, "QueryBuilder::logicOperatorToString()", "Unknown logic operator.");
    return QLatin1StringView("");
}

static QLatin1StringView sortOrderToString(Query::SortOrder order)
{
    switch (order) {
    case Query::Ascending:
        return QLatin1StringView(" ASC");
    case Query::Descending:
        return QLatin1StringView(" DESC");
    }
    Q_ASSERT_X(false, "QueryBuilder::sortOrderToString()", "Unknown sort order.");
    return QLatin1StringView("");
}

static void appendJoined(QString *statement, const QStringList &strings, QLatin1StringView glue = QLatin1StringView(", "))
{
    for (int i = 0, c = strings.size(); i < c; ++i) {
        *statement += strings.at(i);
        if (i + 1 < c) {
            *statement += glue;
        }
    }
}

QueryBuilder::QueryBuilder(const QString &table, QueryBuilder::QueryType type)
    : QueryBuilder(defaultDataStore(), table, type)
{
}

QueryBuilder::QueryBuilder(DataStore *store, const QString &table, QueryBuilder::QueryType type)
    : mTable(table)
#ifndef QUERYBUILDER_UNITTEST
    , mDataStore(store)
    , mDatabaseType(DbType::type(store->database()))
    , mQuery(store->database())
#else
    , mDatabaseType(DbType::Unknown)
#endif
    , mType(type)
    , mLimit(-1)
    , mOffset(-1)
    , mDistinct(false)
{
    static const QString defaultIdColumn = QStringLiteral("id");
    mIdentificationColumn = defaultIdColumn;
}

QueryBuilder::QueryBuilder(const QSqlQuery &tableQuery, const QString &tableQueryAlias)
    : QueryBuilder(defaultDataStore(), tableQuery, tableQueryAlias)
{
}

QueryBuilder::QueryBuilder(DataStore *store, const QSqlQuery &tableQuery, const QString &tableQueryAlias)
    : mTable(tableQueryAlias)
    , mTableSubQuery(tableQuery)
#ifndef QUERYBUILDER_UNITTEST
    , mDataStore(store)
    , mDatabaseType(DbType::type(store->database()))
    , mQuery(store->database())
#else
    , mDatabaseType(DbType::Unknown)
#endif
    , mType(QueryType::Select)
    , mLimit(-1)
    , mOffset(-1)
    , mDistinct(false)
{
    static const QString defaultIdColumn = QStringLiteral("id");
    mIdentificationColumn = defaultIdColumn;
}

void QueryBuilder::setDatabaseType(DbType::Type type)
{
    mDatabaseType = type;
}

void QueryBuilder::addJoin(JoinType joinType, const QString &table, const Query::Condition &condition)
{
    Q_ASSERT((joinType == InnerJoin && (mType == Select || mType == Update)) || (joinType == LeftJoin && mType == Select));

    if (mJoinedTables.contains(table)) {
        // InnerJoin is more restrictive than a LeftJoin, hence use that in doubt
        mJoins[table].first = qMin(joinType, mJoins.value(table).first);
        mJoins[table].second.addCondition(condition);
    } else {
        mJoins[table] = qMakePair(joinType, condition);
        mJoinedTables << table;
    }
}

void QueryBuilder::addJoin(JoinType joinType, const QString &table, const QString &col1, const QString &col2)
{
    Query::Condition condition;
    condition.addColumnCondition(col1, Query::Equals, col2);
    addJoin(joinType, table, condition);
}

void QueryBuilder::addValueCondition(const QString &column, Query::CompareOperator op, const QVariant &value, ConditionType type)
{
    Q_ASSERT(type == WhereCondition || (type == HavingCondition && mType == Select));
    mRootCondition[type].addValueCondition(column, op, value);
}

void QueryBuilder::addColumnCondition(const QString &column, Query::CompareOperator op, const QString &column2, ConditionType type)
{
    Q_ASSERT(type == WhereCondition || (type == HavingCondition && mType == Select));
    mRootCondition[type].addColumnCondition(column, op, column2);
}

QSqlQuery &QueryBuilder::query()
{
    return mQuery;
}

void QueryBuilder::sqliteAdaptUpdateJoin(Query::Condition &condition)
{
    // FIXME: This does not cover all cases by far. It however can handle most
    // (probably all) of the update-join queries we do in Akonadi and convert them
    // properly into a SQLite-compatible query. Better than nothing ;-)

    if (!condition.mSubConditions.isEmpty()) {
        for (int i = condition.mSubConditions.count() - 1; i >= 0; --i) {
            sqliteAdaptUpdateJoin(condition.mSubConditions[i]);
        }
        return;
    }

    QString table;
    if (condition.mColumn.contains(QLatin1Char('.'))) {
        table = condition.mColumn.left(condition.mColumn.indexOf(QLatin1Char('.')));
    } else {
        return;
    }

    if (!mJoinedTables.contains(table)) {
        return;
    }

    const auto &[type, joinCondition] = mJoins.value(table);

    QueryBuilder qb(table, Select);
    qb.addColumn(condition.mColumn);
    qb.addCondition(joinCondition);

    // Convert the subquery to string
    condition.mColumn.reserve(1024);
    condition.mColumn.resize(0);
    condition.mColumn += QLatin1StringView("( ");
    qb.buildQuery(&condition.mColumn);
    condition.mColumn += QLatin1StringView(" )");
}

void QueryBuilder::buildQuery(QString *statement)
{
    // we add the ON conditions of Inner Joins in a Update query here
    // but don't want to change the mRootCondition on each exec().
    Query::Condition whereCondition = mRootCondition[WhereCondition];

    switch (mType) {
    case Select:
        // Enable forward-only on all SELECT queries, since we never need to
        // iterate backwards. This is a memory optimization.
        mQuery.setForwardOnly(true);
        *statement += QLatin1StringView("SELECT ");
        if (mDistinct) {
            *statement += QLatin1StringView("DISTINCT ");
        }
        Q_ASSERT_X(mColumns.count() > 0, "QueryBuilder::exec()", "No columns specified");
        appendJoined(statement, mColumns);
        *statement += QLatin1StringView(" FROM ");
        *statement += mTableSubQuery.isValid()
                    ? getTableQuery(mTableSubQuery, mTable) : mTable;
        for (const QString &joinedTable : std::as_const(mJoinedTables)) {
            const auto &[joinType, joinCond] = mJoins.value(joinedTable);
            switch (joinType) {
            case LeftJoin:
                *statement += QLatin1StringView(" LEFT JOIN ");
                break;
            case InnerJoin:
                *statement += QLatin1StringView(" INNER JOIN ");
                break;
            }
            *statement += joinedTable;
            *statement += QLatin1StringView(" ON ");
            buildWhereCondition(statement, joinCond);
        }
        break;
    case Insert: {
        *statement += QLatin1StringView("INSERT INTO ");
        *statement += mTable;
        *statement += QLatin1StringView(" (");
        for (int i = 0, c = mColumnValues.size(); i < c; ++i) {
            *statement += mColumnValues.at(i).first;
            if (i + 1 < c) {
                *statement += QLatin1StringView(", ");
            }
        }
        *statement += QLatin1StringView(") VALUES (");
        for (int i = 0, c = mColumnValues.size(); i < c; ++i) {
            bindValue(statement, mColumnValues.at(i).second);
            if (i + 1 < c) {
                *statement += QLatin1StringView(", ");
            }
        }
        *statement += QLatin1Char(')');
        if (mDatabaseType == DbType::PostgreSQL && !mIdentificationColumn.isEmpty()) {
            *statement += QLatin1StringView(" RETURNING ") + mIdentificationColumn;
        }
        break;
    }
    case Update: {
        // put the ON condition into the WHERE part of the UPDATE query
        if (mDatabaseType != DbType::Sqlite) {
            for (const QString &table : std::as_const(mJoinedTables)) {
                const auto &[joinType, joinCond] = mJoins.value(table);
                Q_ASSERT(joinType == InnerJoin);
                whereCondition.addCondition(joinCond);
            }
        } else {
            // Note: this will modify the whereCondition
            sqliteAdaptUpdateJoin(whereCondition);
        }

        *statement += QLatin1StringView("UPDATE ");
        *statement += mTable;

        if (mDatabaseType == DbType::MySQL && !mJoinedTables.isEmpty()) {
            // for mysql we list all tables directly
            *statement += QLatin1StringView(", ");
            appendJoined(statement, mJoinedTables);
        }

        *statement += QLatin1StringView(" SET ");
        Q_ASSERT_X(mColumnValues.count() >= 1, "QueryBuilder::exec()", "At least one column needs to be changed");
        for (int i = 0, c = mColumnValues.size(); i < c; ++i) {
            const auto &[column, value] = mColumnValues.at(i);
            *statement += column;
            *statement += QLatin1StringView(" = ");
            bindValue(statement, value);
            if (i + 1 < c) {
                *statement += QLatin1StringView(", ");
            }
        }

        if (mDatabaseType == DbType::PostgreSQL && !mJoinedTables.isEmpty()) {
            // PSQL have this syntax
            // FROM t1 JOIN t2 JOIN ...
            *statement += QLatin1StringView(" FROM ");
            appendJoined(statement, mJoinedTables, QLatin1StringView(" JOIN "));
        }
        break;
    }
    case Delete:
        *statement += QLatin1StringView("DELETE FROM ");
        *statement += mTable;
        break;
    default:
        Q_ASSERT_X(false, "QueryBuilder::exec()", "Unknown enum value");
    }

    if (!whereCondition.isEmpty()) {
        *statement += QLatin1StringView(" WHERE ");
        buildWhereCondition(statement, whereCondition);
    }

    if (!mGroupColumns.isEmpty()) {
        *statement += QLatin1StringView(" GROUP BY ");
        appendJoined(statement, mGroupColumns);
    }

    if (!mRootCondition[HavingCondition].isEmpty()) {
        *statement += QLatin1StringView(" HAVING ");
        buildWhereCondition(statement, mRootCondition[HavingCondition]);
    }

    if (!mSortColumns.isEmpty()) {
        Q_ASSERT_X(mType == Select, "QueryBuilder::exec()", "Order statements are only valid for SELECT queries");
        *statement += QLatin1StringView(" ORDER BY ");
        for (int i = 0, c = mSortColumns.size(); i < c; ++i) {
            const auto &[column, order] = mSortColumns.at(i);
            *statement += column;
            *statement += sortOrderToString(order);
            if (i + 1 < c) {
                *statement += QLatin1StringView(", ");
            }
        }
    }

    if (mLimit > 0) {
        *statement += QLatin1StringView(" LIMIT ") + QString::number(mLimit);
        if (mOffset > 0) {
            *statement += QLatin1StringView(" OFFSET ") + QString::number(mOffset);
        }
    }

    if (mType == Select && mForUpdate) {
        if (mDatabaseType == DbType::Sqlite) {
            // SQLite does not support SELECT ... FOR UPDATE syntax, because it does
            // table-level locking
        } else {
            *statement += QLatin1StringView(" FOR UPDATE");
        }
    }
}

bool QueryBuilder::exec()
{
    QString statement;
    statement.reserve(1024);
    buildQuery(&statement);

#ifndef QUERYBUILDER_UNITTEST
    auto query = QueryCache::query(statement);
    if (query.has_value()) {
        mQuery = *query;
    } else {
        mQuery.clear();
        if (!mQuery.prepare(statement)) {
            qCCritical(AKONADISERVER_LOG) << "DATABASE ERROR while PREPARING QUERY:";
            qCCritical(AKONADISERVER_LOG) << "  Error code:" << mQuery.lastError().nativeErrorCode();
            qCCritical(AKONADISERVER_LOG) << "  DB error: " << mQuery.lastError().databaseText();
            qCCritical(AKONADISERVER_LOG) << "  Error text:" << mQuery.lastError().text();
            qCCritical(AKONADISERVER_LOG) << "  Query:" << statement;
            return false;
        }
        QueryCache::insert(mDataStore->database(), statement, mQuery);
    }

    // too heavy debug info but worths to have from time to time
    // qCDebug(AKONADISERVER_LOG) << "Executing query" << statement;
    bool isBatch = false;
    for (int i = 0; i < mBindValues.count(); ++i) {
        mQuery.bindValue(QLatin1Char(':') + QString::number(i), mBindValues[i]);
        if (!isBatch && static_cast<QMetaType::Type>(mBindValues[i].type()) == QMetaType::QVariantList) {
            isBatch = true;
        }
        // qCDebug(AKONADISERVER_LOG) << QString::fromLatin1( ":%1" ).arg( i ) <<  mBindValues[i];
    }

    bool ret;

    if (StorageDebugger::instance()->isSQLDebuggingEnabled()) {
        QElapsedTimer t;
        t.start();
        if (isBatch) {
            ret = mQuery.execBatch();
        } else {
            ret = mQuery.exec();
        }
        StorageDebugger::instance()->queryExecuted(reinterpret_cast<qint64>(mDataStore), mQuery, t.elapsed());
    } else {
        StorageDebugger::instance()->incSequence();
        if (isBatch) {
            ret = mQuery.execBatch();
        } else {
            ret = mQuery.exec();
        }
    }

    if (!ret) {
        bool needsRetry = false;
        // Handle transaction deadlocks and timeouts by attempting to replay the transaction.
        if (mDatabaseType == DbType::PostgreSQL) {
            const QString dbError = mQuery.lastError().databaseText();
            if (dbError.contains(QLatin1StringView("40P01" /* deadlock_detected */))) {
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
        } else if (mDatabaseType == DbType::Sqlite) {
            const QString lastErrorStr = mQuery.lastError().nativeErrorCode();
            const int error = lastErrorStr.isEmpty() ? -1 : lastErrorStr.toInt();
            if (error == 6 /* SQLITE_LOCKED */) {
                qCWarning(AKONADISERVER_LOG) << "QueryBuilder::exec(): database reported transaction deadlock, retrying transaction";
                qCWarning(AKONADISERVER_LOG) << mQuery.lastError().text();
                mDataStore->doRollback();
                needsRetry = true;
            } else if (error == 5 /* SQLITE_BUSY */) {
                qCWarning(AKONADISERVER_LOG) << "QueryBuilder::exec(): database reported transaction timeout, retrying transaction";
                qCWarning(AKONADISERVER_LOG) << mQuery.lastError().text();
                mDataStore->doRollback();
                needsRetry = true;
            }
        }

        if (needsRetry) {
            mDataStore->transactionKilledByDB();
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
#else
    mStatement = statement;
#endif
    return true;
}

void QueryBuilder::addColumns(const QStringList &cols)
{
    mColumns << cols;
}

void QueryBuilder::addColumn(const QString &col)
{
    mColumns << col;
}

void QueryBuilder::addColumn(const Query::Case &caseStmt)
{
    QString query;
    buildCaseStatement(&query, caseStmt);
    mColumns.append(query);
}

void QueryBuilder::addAggregation(const QString &col, const QString &aggregate)
{
    mColumns.append(aggregate + QLatin1Char('(') + col + QLatin1Char(')'));
}

void QueryBuilder::addAggregation(const Query::Case &caseStmt, const QString &aggregate)
{
    QString query(aggregate + QLatin1Char('('));
    buildCaseStatement(&query, caseStmt);
    query += QLatin1Char(')');

    mColumns.append(query);
}

void QueryBuilder::bindValue(QString *query, const QVariant &value)
{
    mBindValues << value;
    *query += QLatin1Char(':') + QString::number(mBindValues.count() - 1);
}

void QueryBuilder::buildWhereCondition(QString *query, const Query::Condition &cond)
{
    if (!cond.isEmpty()) {
        *query += QLatin1StringView("( ");
        const QLatin1StringView glue = logicOperatorToString(cond.mCombineOp);
        const Query::Condition::List &subConditions = cond.subConditions();
        for (qsizetype i = 0, c = subConditions.size(); i < c; ++i) {
            buildWhereCondition(query, subConditions.at(i));
            if (i + 1 < c) {
                *query += glue;
            }
        }
        *query += QLatin1StringView(" )");
    } else {
        *query += cond.mColumn;
        *query += compareOperatorToString(cond.mCompareOp);
        if (cond.mComparedColumn.isEmpty()) {
            if (cond.mComparedValue.isValid()) {
                if (cond.mComparedValue.canConvert<QVariantList>() && cond.mComparedValue.typeId() != QMetaType::QString
                    && cond.mComparedValue.typeId() != QMetaType::QByteArray) {
                    *query += QLatin1StringView("( ");
                    const QVariantList &entries = cond.mComparedValue.toList();
                    Q_ASSERT_X(!entries.isEmpty(), "QueryBuilder::buildWhereCondition()", "No values given for IN condition.");
                    for (qsizetype i = 0, c = entries.size(); i < c; ++i) {
                        bindValue(query, entries.at(i));
                        if (i + 1 < c) {
                            *query += QLatin1StringView(", ");
                        }
                    }
                    *query += QLatin1StringView(" )");
                } else {
                    bindValue(query, cond.mComparedValue);
                }
            } else {
                *query += QLatin1StringView("NULL");
            }
        } else {
            *query += cond.mComparedColumn;
        }
    }
}

void QueryBuilder::buildCaseStatement(QString *query, const Query::Case &caseStmt)
{
    *query += QLatin1StringView("CASE ");
    for (const auto &whenThen : caseStmt.mWhenThen) {
        *query += QLatin1StringView("WHEN ");
        buildWhereCondition(query, whenThen.first); // When
        *query += QLatin1StringView(" THEN ") + whenThen.second; // then
    }
    if (!caseStmt.mElse.isEmpty()) {
        *query += QLatin1StringView(" ELSE ") + caseStmt.mElse;
    }
    *query += QLatin1StringView(" END");
}

void QueryBuilder::setSubQueryMode(Query::LogicOperator op, ConditionType type)
{
    Q_ASSERT(type == WhereCondition || (type == HavingCondition && mType == Select));
    mRootCondition[type].setSubQueryMode(op);
}

void QueryBuilder::addCondition(const Query::Condition &condition, ConditionType type)
{
    Q_ASSERT(type == WhereCondition || (type == HavingCondition && mType == Select));
    mRootCondition[type].addCondition(condition);
}

void QueryBuilder::addSortColumn(const QString &column, Query::SortOrder order)
{
    mSortColumns << qMakePair(column, order);
}

void QueryBuilder::addGroupColumn(const QString &column)
{
    Q_ASSERT(mType == Select);
    mGroupColumns << column;
}

void QueryBuilder::addGroupColumns(const QStringList &columns)
{
    Q_ASSERT(mType == Select);
    mGroupColumns += columns;
}

void QueryBuilder::setColumnValue(const QString &column, const QVariant &value)
{
    mColumnValues << qMakePair(column, value);
}

void QueryBuilder::setDistinct(bool distinct)
{
    mDistinct = distinct;
}

void QueryBuilder::setLimit(int limit, int offset)
{
    mLimit = limit;
    mOffset = offset;
}

void QueryBuilder::setIdentificationColumn(const QString &column)
{
    mIdentificationColumn = column;
}

qint64 QueryBuilder::insertId()
{
    if (mDatabaseType == DbType::PostgreSQL) {
        query().next();
        if (mIdentificationColumn.isEmpty()) {
            return 0; // FIXME: Does this make sense?
        }
        return query().record().value(mIdentificationColumn).toLongLong();
    } else {
        const QVariant v = query().lastInsertId();
        if (!v.isValid()) {
            return -1;
        }
        bool ok;
        const qint64 insertId = v.toLongLong(&ok);
        if (!ok) {
            return -1;
        }
        return insertId;
    }
    return -1;
}

void QueryBuilder::setForUpdate(bool forUpdate)
{
    mForUpdate = forUpdate;
}

QString QueryBuilder::getTable() const
{
    return mTable;
}

QString QueryBuilder::getTableWithColumn(const QString &column) const
{
    return mTable + QLatin1Char('.') + column;
}

QString QueryBuilder::getTableQuery(const QSqlQuery& query, const QString &alias)
{
    Q_ASSERT_X(query.isValid() && query.isSelect(), "QueryBuilder::getTableQuery", "Table subquery use only for valid SELECT queries");

    QString tableQuery = query.lastQuery();
    if (tableQuery.isEmpty()) {
        qCWarning(AKONADISERVER_LOG) << "Table subquery is empty";
        return tableQuery;
    }

    tableQuery.prepend(QLatin1StringView("( "));

    const QList<QVariant> boundValues = query.boundValues();
    for (qsizetype pos = boundValues.size() - 1; pos >= 0; --pos) {
        const QVariant &value = boundValues.at(pos);
        const QString key(QLatin1Char(':') + QString::number(pos));
        QSqlField field(QLatin1StringView(""), value.metaType());
        if (value.isNull()) {
            field.clear();
        }
        else {
            field.setValue(value);
        }

        const QString formattedValue = query.driver()->formatValue(field);
        tableQuery.replace(key, formattedValue);
    }

    tableQuery.append(QLatin1StringView(" ) AS %1").arg(alias));

    return tableQuery;
}
