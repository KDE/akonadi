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

#include "dbintrospector_impl.h"
#include "dbexception.h"
#include "querybuilder.h"

#include "akonadiserver_debug.h"
#include <QSqlQuery>

using namespace Akonadi::Server;

//BEGIN MySql

DbIntrospectorMySql::DbIntrospectorMySql(const QSqlDatabase &database)
    : DbIntrospector(database)
{
}

QString DbIntrospectorMySql::hasIndexQuery(const QString &tableName, const QString &indexName)
{
    return QString::fromLatin1("SHOW INDEXES FROM %1 WHERE `Key_name` = '%2'")
           .arg(tableName).arg(indexName);
}

QVector< DbIntrospector::ForeignKey > DbIntrospectorMySql::foreignKeyConstraints(const QString &tableName)
{
    QueryBuilder qb(QStringLiteral("information_schema.REFERENTIAL_CONSTRAINTS"), QueryBuilder::Select);
    qb.addJoin(QueryBuilder::InnerJoin, QStringLiteral("information_schema.KEY_COLUMN_USAGE"),
               QStringLiteral("information_schema.REFERENTIAL_CONSTRAINTS.CONSTRAINT_NAME"),
               QStringLiteral("information_schema.KEY_COLUMN_USAGE.CONSTRAINT_NAME"));
    qb.addColumn(QStringLiteral("information_schema.REFERENTIAL_CONSTRAINTS.CONSTRAINT_NAME"));
    qb.addColumn(QStringLiteral("information_schema.KEY_COLUMN_USAGE.COLUMN_NAME"));
    qb.addColumn(QStringLiteral("information_schema.KEY_COLUMN_USAGE.REFERENCED_TABLE_NAME"));
    qb.addColumn(QStringLiteral("information_schema.KEY_COLUMN_USAGE.REFERENCED_COLUMN_NAME"));
    qb.addColumn(QStringLiteral("information_schema.REFERENTIAL_CONSTRAINTS.UPDATE_RULE"));
    qb.addColumn(QStringLiteral("information_schema.REFERENTIAL_CONSTRAINTS.DELETE_RULE"));

    qb.addValueCondition(QStringLiteral("information_schema.KEY_COLUMN_USAGE.TABLE_SCHEMA"), Query::Equals, m_database.databaseName());
    qb.addValueCondition(QStringLiteral("information_schema.KEY_COLUMN_USAGE.TABLE_NAME"), Query::Equals, tableName);

    if (!qb.exec()) {
        throw DbException(qb.query());
    }

    QVector<ForeignKey> result;
    while (qb.query().next()) {
        ForeignKey fk;
        fk.name = qb.query().value(0).toString();
        fk.column = qb.query().value(1).toString();
        fk.refTable = qb.query().value(2).toString();
        fk.refColumn = qb.query().value(3).toString();
        fk.onUpdate = qb.query().value(4).toString();
        fk.onDelete = qb.query().value(5).toString();
        result.push_back(fk);
    }

    return result;
}

//END MySql

//BEGIN Sqlite

DbIntrospectorSqlite::DbIntrospectorSqlite(const QSqlDatabase &database)
    : DbIntrospector(database)
{
}

QString DbIntrospectorSqlite::hasIndexQuery(const QString &tableName, const QString &indexName)
{
    return QString::fromLatin1("SELECT * FROM sqlite_master WHERE type='index' AND tbl_name='%1' AND name='%2';")
           .arg(tableName).arg(indexName);
}

//END Sqlite

//BEGIN PostgreSql

DbIntrospectorPostgreSql::DbIntrospectorPostgreSql(const QSqlDatabase &database)
    : DbIntrospector(database)
{
}

QVector<DbIntrospector::ForeignKey> DbIntrospectorPostgreSql::foreignKeyConstraints(const QString &tableName)
{
#define TABLE_CONSTRAINTS "information_schema.table_constraints"
#define KEY_COLUMN_USAGE "information_schema.key_column_usage"
#define REFERENTIAL_CONSTRAINTS "information_schema.referential_constraints"
#define CONSTRAINT_COLUMN_USAGE "information_schema.constraint_column_usage"

    Query::Condition keyColumnUsageCondition(Query::And);
    keyColumnUsageCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_catalog"), Query::Equals,
                                               QStringLiteral(KEY_COLUMN_USAGE ".constraint_catalog"));
    keyColumnUsageCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_schema"), Query::Equals,
                                               QStringLiteral(KEY_COLUMN_USAGE ".constraint_schema"));
    keyColumnUsageCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_name"), Query::Equals,
                                               QStringLiteral(KEY_COLUMN_USAGE ".constraint_name"));

    Query::Condition referentialConstraintsCondition(Query::And);
    referentialConstraintsCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_catalog"), Query::Equals,
                                                       QStringLiteral(REFERENTIAL_CONSTRAINTS ".constraint_catalog"));
    referentialConstraintsCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_schema"), Query::Equals,
                                                       QStringLiteral(REFERENTIAL_CONSTRAINTS ".constraint_schema"));
    referentialConstraintsCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_name"), Query::Equals,
                                                       QStringLiteral(REFERENTIAL_CONSTRAINTS ".constraint_name"));

    Query::Condition constraintColumnUsageCondition(Query::And);
    constraintColumnUsageCondition.addColumnCondition(QStringLiteral(REFERENTIAL_CONSTRAINTS ".unique_constraint_catalog"), Query::Equals,
                                                      QStringLiteral(CONSTRAINT_COLUMN_USAGE ".constraint_catalog"));
    constraintColumnUsageCondition.addColumnCondition(QStringLiteral(REFERENTIAL_CONSTRAINTS ".unique_constraint_schema"), Query::Equals,
                                                      QStringLiteral(CONSTRAINT_COLUMN_USAGE ".constraint_schema"));
    constraintColumnUsageCondition.addColumnCondition(QStringLiteral(REFERENTIAL_CONSTRAINTS ".unique_constraint_name"), Query::Equals,
                                                      QStringLiteral(CONSTRAINT_COLUMN_USAGE ".constraint_name"));

    QueryBuilder qb(QStringLiteral(TABLE_CONSTRAINTS), QueryBuilder::Select);
    qb.addColumn(QStringLiteral(TABLE_CONSTRAINTS ".constraint_name"));
    qb.addColumn(QStringLiteral(KEY_COLUMN_USAGE ".column_name"));
    qb.addColumn(QStringLiteral(CONSTRAINT_COLUMN_USAGE ".table_name AS referenced_table"));
    qb.addColumn(QStringLiteral(CONSTRAINT_COLUMN_USAGE ".column_name AS referenced_column"));
    qb.addColumn(QStringLiteral(REFERENTIAL_CONSTRAINTS ".update_rule"));
    qb.addColumn(QStringLiteral(REFERENTIAL_CONSTRAINTS ".delete_rule"));
    qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral(KEY_COLUMN_USAGE), keyColumnUsageCondition);
    qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral(REFERENTIAL_CONSTRAINTS), referentialConstraintsCondition);
    qb.addJoin(QueryBuilder::LeftJoin, QStringLiteral(CONSTRAINT_COLUMN_USAGE), constraintColumnUsageCondition);
    qb.addValueCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_type"),
                         Query::Equals, QLatin1String("FOREIGN KEY"));
    qb.addValueCondition(QStringLiteral(TABLE_CONSTRAINTS ".table_name"),
                         Query::Equals, tableName.toLower());

#undef TABLE_CONSTRAINTS
#undef KEY_COLUMN_USAGE
#undef REFERENTIAL_CONSTRAINTS
#undef CONSTRAINT_COLUMN_USAGE

    if (!qb.exec()) {
        throw DbException(qb.query());
    }

    QVector<ForeignKey> result;
    while (qb.query().next()) {
        ForeignKey fk;
        fk.name = qb.query().value(0).toString();
        fk.column = qb.query().value(1).toString();
        fk.refTable = qb.query().value(2).toString();
        fk.refColumn = qb.query().value(3).toString();
        fk.onUpdate = qb.query().value(4).toString();
        fk.onDelete = qb.query().value(5).toString();
        result.push_back(fk);
    }

    return result;
}

QString DbIntrospectorPostgreSql::hasIndexQuery(const QString &tableName, const QString &indexName)
{
    QString query = QStringLiteral("SELECT indexname FROM pg_catalog.pg_indexes");
    query += QString::fromLatin1(" WHERE tablename ilike '%1'").arg(tableName);
    query += QString::fromLatin1(" AND  indexname ilike '%1'").arg(indexName);
    query += QString::fromLatin1(" UNION SELECT conname FROM pg_catalog.pg_constraint ");
    query += QString::fromLatin1(" WHERE conname ilike '%1'").arg(indexName);
    return query;
}

//END PostgreSql
