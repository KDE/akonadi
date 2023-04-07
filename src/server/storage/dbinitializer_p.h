/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *   SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include "storage/dbinitializer.h"

namespace Akonadi
{
namespace Server
{
class DbInitializerMySql : public DbInitializer
{
public:
    explicit DbInitializerMySql(const QSqlDatabase &database);

protected:
    QString sqlType(const ColumnDescription &col, int size) const override;

    QString buildCreateTableStatement(const TableDescription &tableDescription) const override;
    QString buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const override;
    QString buildInsertValuesStatement(const TableDescription &tableDescription, const DataDescription &dataDescription) const override;
    QStringList buildAddForeignKeyConstraintStatements(const TableDescription &table, const ColumnDescription &column) const override;
    QStringList buildRemoveForeignKeyConstraintStatements(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const override;
};

class DbInitializerSqlite : public DbInitializer
{
public:
    explicit DbInitializerSqlite(const QSqlDatabase &database);

protected:
    QString buildCreateTableStatement(const TableDescription &tableDescription) const override;
    QString buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const override;
    QString buildInsertValuesStatement(const TableDescription &tableDescription, const DataDescription &dataDescription) const override;
    QString sqlValue(const ColumnDescription &col, const QString &value) const override;
    QStringList buildAddForeignKeyConstraintStatements(const TableDescription &table, const ColumnDescription &column) const override;
    QStringList buildRemoveForeignKeyConstraintStatements(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const override;

private:
    QStringList buildUpdateForeignKeyConstraintsStatements(const TableDescription &table) const;
};

class DbInitializerPostgreSql : public DbInitializer
{
public:
    explicit DbInitializerPostgreSql(const QSqlDatabase &database);

protected:
    QString sqlType(const ColumnDescription &col, int size) const override;

    QString buildCreateTableStatement(const TableDescription &tableDescription) const override;
    QString buildColumnStatement(const ColumnDescription &columnDescription, const TableDescription &tableDescription) const override;
    QString buildInsertValuesStatement(const TableDescription &tableDescription, const DataDescription &dataDescription) const override;
    QStringList buildAddForeignKeyConstraintStatements(const TableDescription &table, const ColumnDescription &column) const override;
    QStringList buildRemoveForeignKeyConstraintStatements(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const override;
};

} // namespace Server
} // namespace Akonadi
