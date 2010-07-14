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

class DbInitializerMySql : public DbInitializer
{
  public:
    DbInitializerMySql(const QSqlDatabase& database, const QString& templateFile);
  protected:
    QString sqlType(const QString& type) const;
    QString hasIndexQuery(const QString& tableName, const QString& indexName);

    virtual QString buildCreateTableStatement( const TableDescription &tableDescription ) const;
    virtual QString buildColumnStatement( const ColumnDescription &columnDescription ) const;
    virtual QString buildInsertValuesStatement( const TableDescription &tableDescription, const DataDescription &dataDescription ) const;
};

class DbInitializerSqlite : public DbInitializer
{
  public:
    DbInitializerSqlite( const QSqlDatabase& database, const QString& templateFile);
  protected:
    QString hasIndexQuery(const QString& tableName, const QString& indexName);

    virtual QString buildCreateTableStatement( const TableDescription &tableDescription ) const;
    virtual QString buildColumnStatement( const ColumnDescription &columnDescription ) const;
    virtual QString buildInsertValuesStatement( const TableDescription &tableDescription, const DataDescription &dataDescription ) const;
};

class DbInitializerPostgreSql : public DbInitializer
{
  public:
    DbInitializerPostgreSql(const QSqlDatabase& database, const QString& templateFile);
  protected:
    QString sqlType(const QString& type) const;
    QString hasIndexQuery(const QString& tableName, const QString& indexName);

    virtual QString buildCreateTableStatement( const TableDescription &tableDescription ) const;
    virtual QString buildColumnStatement( const ColumnDescription &columnDescription ) const;
    virtual QString buildInsertValuesStatement( const TableDescription &tableDescription, const DataDescription &dataDescription ) const;
    virtual QString buildCreateRelationTableStatement( const QString &tableName, const RelationDescription &relationDescription ) const;
};

class DbInitializerVirtuoso : public DbInitializer
{
  public:
    DbInitializerVirtuoso(const QSqlDatabase& database, const QString& templateFile);
  protected:
    QString sqlType(const QString& type) const;
    QString sqlValue(const QString& type, const QString& value) const;
    bool hasIndex(const QString& tableName, const QString& indexName);

    virtual QString buildCreateTableStatement( const TableDescription &tableDescription ) const;
    virtual QString buildColumnStatement( const ColumnDescription &columnDescription ) const;
    virtual QString buildInsertValuesStatement( const TableDescription &tableDescription, const DataDescription &dataDescription ) const;
};

#endif
