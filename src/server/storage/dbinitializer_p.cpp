/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "storage/dbinitializer_p.h"
#include <shared/akranges.h>

using namespace Akonadi;
using namespace Akonadi::Server;

// BEGIN MySQL

DbInitializerMySql::DbInitializerMySql(const QSqlDatabase &database)
    : DbInitializer(database)
{
}

QString DbInitializerMySql::sqlType(const ColumnDescription &col, int size) const
{
    if (col.type == QLatin1StringView("QString")) {
        return QLatin1StringView("VARBINARY(") + QString::number(size <= 0 ? 255 : size) + QLatin1String(")");
    } else {
        return DbInitializer::sqlType(col, size);
    }
}

QString DbInitializerMySql::buildCreateTableStatement(const TableDescription &tableDescription) const
{
    QStringList columns;
    QStringList references;

    for (const ColumnDescription &columnDescription : tableDescription.columns) {
        columns.append(buildColumnStatement(columnDescription, tableDescription));

        if (!columnDescription.refTable.isEmpty() && !columnDescription.refColumn.isEmpty()) {
            references << QStringLiteral("FOREIGN KEY (%1) REFERENCES %2Table(%3) ")
                              .arg(columnDescription.name, columnDescription.refTable, columnDescription.refColumn)
                    + buildReferentialAction(columnDescription.onUpdate, columnDescription.onDelete);
        }
    }

    if (tableDescription.primaryKeyColumnCount() > 1) {
        columns.push_back(buildPrimaryKeyStatement(tableDescription));
    }
    columns << references;

    QString tableProperties = QStringLiteral(" COLLATE=utf8_general_ci DEFAULT CHARSET=utf8");
    if (tableDescription.columns | AkRanges::Actions::any([](const auto &col) {
            return col.type == QLatin1StringView("QString") && col.size > 255;
        })) {
        tableProperties += QStringLiteral(" ROW_FORMAT=DYNAMIC");
    }

    return QStringLiteral("CREATE TABLE %1 (%2) %3").arg(tableDescription.name, columns.join(QStringLiteral(", ")), tableProperties);
}

QString DbInitializerMySql::buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const
{
    QString column = columnDescription.name;

    column += QLatin1Char(' ') + sqlType(columnDescription, columnDescription.size);

    if (!columnDescription.allowNull) {
        column += QLatin1StringView(" NOT NULL");
    }

    if (columnDescription.isAutoIncrement) {
        column += QLatin1StringView(" AUTO_INCREMENT");
    }

    if (columnDescription.isPrimaryKey && tableDescription.primaryKeyColumnCount() == 1) {
        column += QLatin1StringView(" PRIMARY KEY");
    }

    if (columnDescription.isUnique) {
        column += QLatin1StringView(" UNIQUE");
    }

    if (!columnDescription.defaultValue.isEmpty()) {
        const QString defaultValue = sqlValue(columnDescription, columnDescription.defaultValue);

        if (!defaultValue.isEmpty()) {
            column += QStringLiteral(" DEFAULT %1").arg(defaultValue);
        }
    }

    return column;
}

QString DbInitializerMySql::buildInsertValuesStatement(const TableDescription &tableDescription, const DataDescription &dataDescription) const
{
    QMap<QString, QString> data = dataDescription.data;
    QStringList keys;
    QStringList values;
    for (auto it = data.begin(), end = data.end(); it != end; ++it) {
        it.value().replace(QLatin1StringView("\\"), QLatin1String("\\\\"));
        keys.push_back(it.key());
        values.push_back(it.value());
    }

    return QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)").arg(tableDescription.name, keys.join(QLatin1Char(',')), values.join(QLatin1Char(',')));
}

QStringList DbInitializerMySql::buildAddForeignKeyConstraintStatements(const TableDescription &table, const ColumnDescription &column) const
{
    return {QStringLiteral("ALTER TABLE %1 ADD FOREIGN KEY (%2) REFERENCES %4Table(%5) %6")
                .arg(table.name, column.name, column.refTable, column.refColumn, buildReferentialAction(column.onUpdate, column.onDelete))};
}

QStringList DbInitializerMySql::buildRemoveForeignKeyConstraintStatements(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const
{
    return {QStringLiteral("ALTER TABLE %1 DROP FOREIGN KEY %2").arg(table.name, fk.name)};
}

// END MySQL

// BEGIN Sqlite

DbInitializerSqlite::DbInitializerSqlite(const QSqlDatabase &database)
    : DbInitializer(database)
{
}

QString DbInitializerSqlite::buildCreateTableStatement(const TableDescription &tableDescription) const
{
    QStringList columns;

    columns.reserve(tableDescription.columns.count() + 1);
    for (const ColumnDescription &columnDescription : std::as_const(tableDescription.columns)) {
        columns.append(buildColumnStatement(columnDescription, tableDescription));
    }

    if (tableDescription.primaryKeyColumnCount() > 1) {
        columns.push_back(buildPrimaryKeyStatement(tableDescription));
    }
    QStringList references;
    for (const ColumnDescription &columnDescription : std::as_const(tableDescription.columns)) {
        if (!columnDescription.refTable.isEmpty() && !columnDescription.refColumn.isEmpty()) {
            const auto constraintName =
                QStringLiteral("%1%2_%3%4_fk").arg(tableDescription.name, columnDescription.name, columnDescription.refTable, columnDescription.refColumn);
            references << QStringLiteral("CONSTRAINT %1 FOREIGN KEY (%2) REFERENCES %3Table(%4) %5 DEFERRABLE INITIALLY DEFERRED")
                              .arg(constraintName,
                                   columnDescription.name,
                                   columnDescription.refTable,
                                   columnDescription.refColumn,
                                   buildReferentialAction(columnDescription.onUpdate, columnDescription.onDelete));
        }
    }
    columns << references;

    return QStringLiteral("CREATE TABLE %1 (%2)").arg(tableDescription.name, columns.join(QStringLiteral(", ")));
}

QString DbInitializerSqlite::buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const
{
    QString column = columnDescription.name + QLatin1Char(' ');

    if (columnDescription.isAutoIncrement) {
        column += QLatin1StringView("INTEGER");
    } else {
        column += sqlType(columnDescription, columnDescription.size);
    }

    if (columnDescription.isPrimaryKey && tableDescription.primaryKeyColumnCount() == 1) {
        column += QLatin1StringView(" PRIMARY KEY");
    } else if (columnDescription.isUnique) {
        column += QLatin1StringView(" UNIQUE");
    }

    if (columnDescription.isAutoIncrement) {
        column += QLatin1StringView(" AUTOINCREMENT");
    }

    if (!columnDescription.allowNull) {
        column += QLatin1StringView(" NOT NULL");
    }

    if (!columnDescription.defaultValue.isEmpty()) {
        const QString defaultValue = sqlValue(columnDescription, columnDescription.defaultValue);

        if (!defaultValue.isEmpty()) {
            column += QStringLiteral(" DEFAULT %1").arg(defaultValue);
        }
    }

    return column;
}

QString DbInitializerSqlite::buildInsertValuesStatement(const TableDescription &tableDescription, const DataDescription &dataDescription) const
{
    QMap<QString, QString> data = dataDescription.data;
    QStringList keys;
    QStringList values;
    for (auto it = data.begin(), end = data.end(); it != end; ++it) {
        it.value().replace(QLatin1StringView("true"), QLatin1String("1"));
        it.value().replace(QLatin1StringView("false"), QLatin1String("0"));
        keys.push_back(it.key());
        values.push_back(it.value());
    }

    return QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)").arg(tableDescription.name, keys.join(QLatin1Char(',')), values.join(QLatin1Char(',')));
}

QString DbInitializerSqlite::sqlValue(const ColumnDescription &col, const QString &value) const
{
    if (col.type == QLatin1StringView("bool")) {
        if (value == QLatin1StringView("false")) {
            return QStringLiteral("0");
        } else if (value == QLatin1StringView("true")) {
            return QStringLiteral("1");
        }
        return value;
    }

    return Akonadi::Server::DbInitializer::sqlValue(col, value);
}

QStringList DbInitializerSqlite::buildAddForeignKeyConstraintStatements(const TableDescription &table, const ColumnDescription & /*column*/) const
{
    return buildUpdateForeignKeyConstraintsStatements(table);
}

QStringList DbInitializerSqlite::buildRemoveForeignKeyConstraintStatements(const DbIntrospector::ForeignKey & /*fk*/, const TableDescription &table) const
{
    return buildUpdateForeignKeyConstraintsStatements(table);
}

QStringList DbInitializerSqlite::buildUpdateForeignKeyConstraintsStatements(const TableDescription &table) const
{
    // Unfortunately, SQLite does not support adding or removing foreign keys through ALTER TABLE,
    // this is the only way how to do it.
    return {QStringLiteral("PRAGMA defer_foreign_keys=ON"),
            QStringLiteral("BEGIN TRANSACTION"),
            QStringLiteral("ALTER TABLE %1 RENAME TO %1_old").arg(table.name),
            buildCreateTableStatement(table),
            QStringLiteral("INSERT INTO %1 SELECT * FROM %1_old").arg(table.name),
            QStringLiteral("DROP TABLE %1_old").arg(table.name),
            QStringLiteral("COMMIT"),
            QStringLiteral("PRAGMA defer_foreign_keys=OFF")};
}

// END Sqlite

// BEGIN PostgreSQL

DbInitializerPostgreSql::DbInitializerPostgreSql(const QSqlDatabase &database)
    : DbInitializer(database)
{
}

QString DbInitializerPostgreSql::sqlType(const ColumnDescription &col, int size) const
{
    if (col.type == QLatin1StringView("qint64")) {
        return QStringLiteral("int8");
    } else if (col.type == QLatin1StringView("QByteArray")) {
        return QStringLiteral("BYTEA");
    } else if (col.isEnum) {
        return QStringLiteral("SMALLINT");
    }

    return DbInitializer::sqlType(col, size);
}

QString DbInitializerPostgreSql::buildCreateTableStatement(const TableDescription &tableDescription) const
{
    QStringList columns;
    columns.reserve(tableDescription.columns.size() + 1);

    for (const ColumnDescription &columnDescription : tableDescription.columns) {
        columns.append(buildColumnStatement(columnDescription, tableDescription));
    }

    if (tableDescription.primaryKeyColumnCount() > 1) {
        columns.push_back(buildPrimaryKeyStatement(tableDescription));
    }

    return QStringLiteral("CREATE TABLE %1 (%2)").arg(tableDescription.name, columns.join(QStringLiteral(", ")));
}

QString DbInitializerPostgreSql::buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const
{
    QString column = columnDescription.name + QLatin1Char(' ');

    if (columnDescription.isAutoIncrement) {
        column += QLatin1StringView("SERIAL");
    } else {
        column += sqlType(columnDescription, columnDescription.size);
    }

    if (columnDescription.isPrimaryKey && tableDescription.primaryKeyColumnCount() == 1) {
        column += QLatin1StringView(" PRIMARY KEY");
    } else if (columnDescription.isUnique) {
        column += QLatin1StringView(" UNIQUE");
    }

    if (!columnDescription.allowNull && !(columnDescription.isPrimaryKey && tableDescription.primaryKeyColumnCount() == 1)) {
        column += QLatin1StringView(" NOT NULL");
    }

    if (!columnDescription.defaultValue.isEmpty()) {
        const QString defaultValue = sqlValue(columnDescription, columnDescription.defaultValue);

        if (!defaultValue.isEmpty()) {
            column += QStringLiteral(" DEFAULT %1").arg(defaultValue);
        }
    }

    return column;
}

QString DbInitializerPostgreSql::buildInsertValuesStatement(const TableDescription &tableDescription, const DataDescription &dataDescription) const
{
    QStringList keys;
    QStringList values;
    for (auto it = dataDescription.data.cbegin(), end = dataDescription.data.cend(); it != end; ++it) {
        keys.push_back(it.key());
        values.push_back(it.value());
    }

    return QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)").arg(tableDescription.name, keys.join(QLatin1Char(',')), values.join(QLatin1Char(',')));
}

QStringList DbInitializerPostgreSql::buildAddForeignKeyConstraintStatements(const TableDescription &table, const ColumnDescription &column) const
{
    // constraints must have name in PostgreSQL
    const QString constraintName = table.name + column.name + QLatin1StringView("_") + column.refTable + column.refColumn + QLatin1String("_fk");
    return {QStringLiteral("ALTER TABLE %1 ADD CONSTRAINT %2 FOREIGN KEY (%3) REFERENCES %4Table(%5) %6 DEFERRABLE INITIALLY DEFERRED")
                .arg(table.name, constraintName, column.name, column.refTable, column.refColumn, buildReferentialAction(column.onUpdate, column.onDelete))};
}

QStringList DbInitializerPostgreSql::buildRemoveForeignKeyConstraintStatements(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const
{
    return {QStringLiteral("ALTER TABLE %1 DROP CONSTRAINT %2").arg(table.name, fk.name)};
}

// END PostgreSQL
