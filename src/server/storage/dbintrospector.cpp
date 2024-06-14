/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbintrospector.h"
#include "akonadiserver_debug.h"
#include "datastore.h"
#include "dbexception.h"
#include "dbintrospector_impl.h"
#include "dbtype.h"
#include "querybuilder.h"

#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>

using namespace Akonadi::Server;

DbIntrospector::Ptr DbIntrospector::createInstance(const QSqlDatabase &database)
{
    switch (DbType::type(database)) {
    case DbType::MySQL:
        return Ptr(new DbIntrospectorMySql(database));
    case DbType::Sqlite:
        return Ptr(new DbIntrospectorSqlite(database));
    case DbType::PostgreSQL:
        return Ptr(new DbIntrospectorPostgreSql(database));
    case DbType::Unknown:
        break;
    }
    qCCritical(AKONADISERVER_LOG) << database.driverName() << "backend  not supported";
    return Ptr();
}

DbIntrospector::DbIntrospector(const QSqlDatabase &database)
    : m_database(database)
{
}

DbIntrospector::~DbIntrospector()
{
}

bool DbIntrospector::hasTable(const QString &tableName)
{
    return m_database.tables().contains(tableName, Qt::CaseInsensitive);
}

bool DbIntrospector::hasIndex(const QString &tableName, const QString &indexName)
{
    QSqlQuery query(m_database);
    if (!query.exec(hasIndexQuery(tableName, indexName))) {
        throw DbException(query, "Failed to query index");
    }
    return query.next();
}

bool DbIntrospector::hasColumn(const QString &tableName, const QString &columnName)
{
    QStringList columns = m_columnCache.value(tableName);

    if (columns.isEmpty()) {
        // QPSQL requires the name to be lower case, but it breaks compatibility with existing
        // tables with other drivers (see BKO#409234). Yay for abstraction...
        const auto name = (DbType::type(m_database) == DbType::PostgreSQL) ? tableName.toLower() : tableName;
        const QSqlRecord table = m_database.record(name);
        const int numTables = table.count();
        columns.reserve(numTables);
        for (int i = 0; i < numTables; ++i) {
            const QSqlField column = table.field(i);
            columns.push_back(column.name().toLower());
        }

        m_columnCache.insert(tableName, columns);
    }

    return columns.contains(columnName.toLower());
}

bool DbIntrospector::isTableEmpty(const QString &tableName)
{
    auto *store = DataStore::dataStoreForDatabase(m_database);
    QueryBuilder queryBuilder(store, tableName, QueryBuilder::Select);
    queryBuilder.addColumn(QStringLiteral("*"));
    queryBuilder.setLimit(1);
    if (!queryBuilder.exec()) {
        throw DbException(queryBuilder.takeQuery(), "Unable to retrieve data from table.");
    }

    QSqlQuery query = queryBuilder.takeQuery();
    return (query.size() == 0 || !query.first());
}

QList<DbIntrospector::ForeignKey> DbIntrospector::foreignKeyConstraints(const QString &tableName)
{
    Q_UNUSED(tableName)
    return QList<ForeignKey>();
}

QString DbIntrospector::hasIndexQuery(const QString &tableName, const QString &indexName)
{
    Q_UNUSED(tableName)
    Q_UNUSED(indexName)
    qCCritical(AKONADISERVER_LOG) << "Implement index support for your database!";
    return QString();
}
