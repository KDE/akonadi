/*
    Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>
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

#include "dbintrospector.h"
#include "dbintrospector_impl.h"
#include "dbtype.h"
#include "dbexception.h"
#include "querybuilder.h"

#include <shared/akdebug.h>

#include <QStringList>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlQuery>

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
    akFatal() << database.driverName() << "backend  not supported";
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
        const QSqlRecord table = m_database.record(tableName);
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
    QueryBuilder queryBuilder(tableName, QueryBuilder::Select);
    queryBuilder.addColumn(QStringLiteral("*"));
    queryBuilder.setLimit(1);
    if (!queryBuilder.exec()) {
        throw DbException(queryBuilder.query(), "Unable to retrieve data from table.");
    }

    QSqlQuery query = queryBuilder.query();
    if (query.size() == 0  || !query.first()) {   // table is empty
        return true;
    }
    return false;
}

QVector<DbIntrospector::ForeignKey> DbIntrospector::foreignKeyConstraints(const QString &tableName)
{
    Q_UNUSED(tableName);
    return QVector<ForeignKey>();
}

QString DbIntrospector::hasIndexQuery(const QString &tableName, const QString &indexName)
{
    Q_UNUSED(tableName);
    Q_UNUSED(indexName);
    akFatal() << "Implement index support for your database!";
    return QString();
}
