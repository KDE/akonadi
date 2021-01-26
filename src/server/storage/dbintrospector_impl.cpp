/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbintrospector_impl.h"
#include "datastore.h"
#include "dbexception.h"
#include "querybuilder.h"

#include "akonadiserver_debug.h"

using namespace Akonadi::Server;

// BEGIN MySql

DbIntrospectorMySql::DbIntrospectorMySql(const QSqlDatabase &database)
    : DbIntrospector(database)
{
}

QString DbIntrospectorMySql::hasIndexQuery(const QString &tableName, const QString &indexName)
{
    return QStringLiteral("SHOW INDEXES FROM %1 WHERE `Key_name` = '%2'").arg(tableName, indexName);
}

QVector<DbIntrospector::ForeignKey> DbIntrospectorMySql::foreignKeyConstraints(const QString &tableName)
{
    QueryBuilder qb(QStringLiteral("information_schema.REFERENTIAL_CONSTRAINTS"), QueryBuilder::Select);
    qb.addJoin(QueryBuilder::InnerJoin,
               QStringLiteral("information_schema.KEY_COLUMN_USAGE"),
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
    qb.query().finish();

    return result;
}

// END MySql

// BEGIN Sqlite

DbIntrospectorSqlite::DbIntrospectorSqlite(const QSqlDatabase &database)
    : DbIntrospector(database)
{
}

QVector<DbIntrospector::ForeignKey> DbIntrospectorSqlite::foreignKeyConstraints(const QString &tableName)
{
    QSqlQuery query(DataStore::self()->database());
    if (!query.exec(QStringLiteral("PRAGMA foreign_key_list(%1)").arg(tableName))) {
        throw DbException(query);
    }

    QVector<ForeignKey> result;
    while (query.next()) {
        ForeignKey fk;
        fk.column = query.value(3).toString();
        fk.refTable = query.value(2).toString();
        fk.refColumn = query.value(4).toString();
        fk.onUpdate = query.value(5).toString();
        fk.onDelete = query.value(6).toString();
        fk.name = tableName + fk.column + QLatin1Char('_') + fk.refTable + fk.refColumn + QStringLiteral("_fk");
        result.push_back(fk);
    }

    return result;
}

QString DbIntrospectorSqlite::hasIndexQuery(const QString &tableName, const QString &indexName)
{
    return QStringLiteral("SELECT * FROM sqlite_master WHERE type='index' AND tbl_name='%1' AND name='%2';").arg(tableName, indexName);
}

// END Sqlite

// BEGIN PostgreSql

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
    keyColumnUsageCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_catalog"),
                                               Query::Equals,
                                               QStringLiteral(KEY_COLUMN_USAGE ".constraint_catalog"));
    keyColumnUsageCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_schema"),
                                               Query::Equals,
                                               QStringLiteral(KEY_COLUMN_USAGE ".constraint_schema"));
    keyColumnUsageCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_name"),
                                               Query::Equals,
                                               QStringLiteral(KEY_COLUMN_USAGE ".constraint_name"));

    Query::Condition referentialConstraintsCondition(Query::And);
    referentialConstraintsCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_catalog"),
                                                       Query::Equals,
                                                       QStringLiteral(REFERENTIAL_CONSTRAINTS ".constraint_catalog"));
    referentialConstraintsCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_schema"),
                                                       Query::Equals,
                                                       QStringLiteral(REFERENTIAL_CONSTRAINTS ".constraint_schema"));
    referentialConstraintsCondition.addColumnCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_name"),
                                                       Query::Equals,
                                                       QStringLiteral(REFERENTIAL_CONSTRAINTS ".constraint_name"));

    Query::Condition constraintColumnUsageCondition(Query::And);
    constraintColumnUsageCondition.addColumnCondition(QStringLiteral(REFERENTIAL_CONSTRAINTS ".unique_constraint_catalog"),
                                                      Query::Equals,
                                                      QStringLiteral(CONSTRAINT_COLUMN_USAGE ".constraint_catalog"));
    constraintColumnUsageCondition.addColumnCondition(QStringLiteral(REFERENTIAL_CONSTRAINTS ".unique_constraint_schema"),
                                                      Query::Equals,
                                                      QStringLiteral(CONSTRAINT_COLUMN_USAGE ".constraint_schema"));
    constraintColumnUsageCondition.addColumnCondition(QStringLiteral(REFERENTIAL_CONSTRAINTS ".unique_constraint_name"),
                                                      Query::Equals,
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
    qb.addValueCondition(QStringLiteral(TABLE_CONSTRAINTS ".constraint_type"), Query::Equals, QLatin1String("FOREIGN KEY"));
    qb.addValueCondition(QStringLiteral(TABLE_CONSTRAINTS ".table_name"), Query::Equals, tableName.toLower());

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
    qb.query().finish();

    return result;
}

QString DbIntrospectorPostgreSql::hasIndexQuery(const QString &tableName, const QString &indexName)
{
    QString query = QStringLiteral("SELECT indexname FROM pg_catalog.pg_indexes");
    query += QStringLiteral(" WHERE tablename ilike '%1'").arg(tableName);
    query += QStringLiteral(" AND  indexname ilike '%1'").arg(indexName);
    query += QStringLiteral(" UNION SELECT conname FROM pg_catalog.pg_constraint ");
    query += QStringLiteral(" WHERE conname ilike '%1'").arg(indexName);
    return query;
}

// END PostgreSql
