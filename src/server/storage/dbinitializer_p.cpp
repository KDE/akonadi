/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2010 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "storage/dbinitializer_p.h"

using namespace Akonadi::Server;

//BEGIN MySQL

DbInitializerMySql::DbInitializerMySql(const QSqlDatabase &database)
    : DbInitializer(database)
{
}

QString DbInitializerMySql::sqlType(const ColumnDescription &col, int size) const
{
    if (col.type == QLatin1String("QString")) {
        return QLatin1Literal("VARBINARY(") + QString::number(size <= 0 ? 255 : size) + QLatin1Literal(")");
    } else {
        return DbInitializer::sqlType(col, size);
    }
}

QString DbInitializerMySql::buildCreateTableStatement(const TableDescription &tableDescription) const
{
    QStringList columns;
    QStringList references;

    Q_FOREACH (const ColumnDescription &columnDescription, tableDescription.columns) {
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

    const QString tableProperties = QStringLiteral(" COLLATE=utf8_general_ci DEFAULT CHARSET=utf8");

    return QStringLiteral("CREATE TABLE %1 (%2) %3").arg(tableDescription.name, columns.join(QStringLiteral(", ")), tableProperties);
}

QString DbInitializerMySql::buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const
{
    QString column = columnDescription.name;

    column += QLatin1Char(' ') + sqlType(columnDescription, columnDescription.size);

    if (!columnDescription.allowNull) {
        column += QLatin1String(" NOT NULL");
    }

    if (columnDescription.isAutoIncrement) {
        column += QLatin1String(" AUTO_INCREMENT");
    }

    if (columnDescription.isPrimaryKey && tableDescription.primaryKeyColumnCount() == 1) {
        column += QLatin1String(" PRIMARY KEY");
    }

    if (columnDescription.isUnique) {
        column += QLatin1String(" UNIQUE");
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
    QMutableMapIterator<QString, QString> it(data);
    while (it.hasNext()) {
        it.next();
        it.value().replace(QLatin1String("\\"), QLatin1String("\\\\"));
    }

    return QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
           .arg(tableDescription.name,
                QStringList(data.keys()).join(QLatin1Char(',')),
                QStringList(data.values()).join(QLatin1Char(',')));
}

QString DbInitializerMySql::buildAddForeignKeyConstraintStatement(const TableDescription &table, const ColumnDescription &column) const
{
    return QLatin1Literal("ALTER TABLE ") + table.name + QLatin1Literal(" ADD FOREIGN KEY (") + column.name
           + QLatin1Literal(") REFERENCES ") + column.refTable + QLatin1Literal("Table(") + column.refColumn
           + QLatin1Literal(") ") + buildReferentialAction(column.onUpdate, column.onDelete);
}

QString DbInitializerMySql::buildRemoveForeignKeyConstraintStatement(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const
{
    return QLatin1Literal("ALTER TABLE ") + table.name + QLatin1Literal(" DROP FOREIGN KEY ") + fk.name;
}

//END MySQL

//BEGIN Sqlite

DbInitializerSqlite::DbInitializerSqlite(const QSqlDatabase &database)
    : DbInitializer(database)
{
}

QString DbInitializerSqlite::buildCreateTableStatement(const TableDescription &tableDescription) const
{
    QStringList columns;

    Q_FOREACH (const ColumnDescription &columnDescription, tableDescription.columns) {
        columns.append(buildColumnStatement(columnDescription, tableDescription));
    }

    if (tableDescription.primaryKeyColumnCount() > 1) {
        columns.push_back(buildPrimaryKeyStatement(tableDescription));
    }

    return QStringLiteral("CREATE TABLE %1 (%2)").arg(tableDescription.name, columns.join(QStringLiteral(", ")));
}

QString DbInitializerSqlite::buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const
{
    QString column = columnDescription.name + QLatin1Char(' ');

    if (columnDescription.isAutoIncrement) {
        column += QLatin1String("INTEGER");
    } else {
        column += sqlType(columnDescription, columnDescription.size);
    }

    if (columnDescription.isPrimaryKey && tableDescription.primaryKeyColumnCount() == 1) {
        column += QLatin1String(" PRIMARY KEY");
    } else if (columnDescription.isUnique) {
        column += QLatin1String(" UNIQUE");
    }

    if (columnDescription.isAutoIncrement) {
        column += QLatin1String(" AUTOINCREMENT");
    }

    if (!columnDescription.allowNull) {
        column += QLatin1String(" NOT NULL");
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
    QMutableMapIterator<QString, QString> it(data);
    while (it.hasNext()) {
        it.next();
        it.value().replace(QLatin1String("true"), QLatin1String("1"));
        it.value().replace(QLatin1String("false"), QLatin1String("0"));
    }

    return QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
           .arg(tableDescription.name,
                QStringList(data.keys()).join(QLatin1Char(',')),
                QStringList(data.values()).join(QLatin1Char(',')));
}

QString DbInitializerSqlite::sqlValue(const ColumnDescription &col, const QString &value) const
{
    if (col.type == QLatin1String("bool")) {
        if (value == QLatin1String("false")) {
            return QStringLiteral("0");
        } else if (value == QLatin1String("true")) {
            return QStringLiteral("1");
        }
        return value;
    }

    return Akonadi::Server::DbInitializer::sqlValue(col, value);
}

//END Sqlite

//BEGIN PostgreSQL

DbInitializerPostgreSql::DbInitializerPostgreSql(const QSqlDatabase &database)
    : DbInitializer(database)
{
}

QString DbInitializerPostgreSql::sqlType(const ColumnDescription &col, int size) const
{
    if (col.type == QLatin1String("qint64")) {
        return QStringLiteral("int8");
    } else if (col.type == QLatin1String("QByteArray")) {
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

    Q_FOREACH (const ColumnDescription &columnDescription, tableDescription.columns) {
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
        column += QLatin1String("SERIAL");
    } else {
        column += sqlType(columnDescription, columnDescription.size);
    }

    if (columnDescription.isPrimaryKey && tableDescription.primaryKeyColumnCount() == 1) {
        column += QLatin1String(" PRIMARY KEY");
    } else if (columnDescription.isUnique) {
        column += QLatin1String(" UNIQUE");
    }

    if (!columnDescription.allowNull && !(columnDescription.isPrimaryKey && tableDescription.primaryKeyColumnCount() == 1)) {
        column += QLatin1String(" NOT NULL");
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
    QMap<QString, QString> data = dataDescription.data;

    return QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
           .arg(tableDescription.name,
                QStringList(data.keys()).join(QLatin1Char(',')),
                QStringList(data.values()).join(QLatin1Char(',')));
}

QString DbInitializerPostgreSql::buildAddForeignKeyConstraintStatement(const TableDescription &table, const ColumnDescription &column) const
{
    // constraints must have name in PostgreSQL
    const QString constraintName = table.name + column.name + QLatin1Literal("_") + column.refTable + column.refColumn + QLatin1Literal("_fk");
    return QLatin1Literal("ALTER TABLE ") + table.name + QLatin1Literal(" ADD CONSTRAINT ") + constraintName + QLatin1Literal(" FOREIGN KEY (") + column.name
           + QLatin1Literal(") REFERENCES ") + column.refTable + QLatin1Literal("Table(") + column.refColumn
           + QLatin1Literal(") ") + buildReferentialAction(column.onUpdate, column.onDelete)
           + QLatin1Literal(" DEFERRABLE INITIALLY DEFERRED");
}

QString DbInitializerPostgreSql::buildRemoveForeignKeyConstraintStatement(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const
{
    return QLatin1Literal("ALTER TABLE ") + table.name + QLatin1Literal(" DROP CONSTRAINT ") + fk.name;
}

//END PostgreSQL
