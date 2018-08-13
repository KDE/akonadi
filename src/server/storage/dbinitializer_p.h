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

#ifndef DBINITIALIZER_P_H
#define DBINITIALIZER_P_H

#include "storage/dbinitializer.h"

namespace Akonadi
{
namespace Server
{

class DbInitializerMySql : public DbInitializer
{
public:
    explicit DbInitializerMySql(const QSqlDatabase &database);

    bool hasForeignKeyConstraints() const override;
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

    bool hasForeignKeyConstraints() const override;
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

    bool hasForeignKeyConstraints() const override;
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

#endif
