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

#ifndef DBINTROSPECTOR_IMPL_H
#define DBINTROSPECTOR_IMPL_H

#include "dbintrospector.h"

namespace Akonadi
{
namespace Server
{

class DbIntrospectorMySql : public DbIntrospector
{
public:
    explicit DbIntrospectorMySql(const QSqlDatabase &database);
    QVector<ForeignKey> foreignKeyConstraints(const QString &tableName) Q_DECL_OVERRIDE;
    QString hasIndexQuery(const QString &tableName, const QString &indexName) Q_DECL_OVERRIDE;
};

class DbIntrospectorSqlite : public DbIntrospector
{
public:
    explicit DbIntrospectorSqlite(const QSqlDatabase &database);
    QString hasIndexQuery(const QString &tableName, const QString &indexName) Q_DECL_OVERRIDE;
};

class DbIntrospectorPostgreSql : public DbIntrospector
{
public:
    explicit DbIntrospectorPostgreSql(const QSqlDatabase &database);
    QVector<ForeignKey> foreignKeyConstraints(const QString &tableName) Q_DECL_OVERRIDE;
    QString hasIndexQuery(const QString &tableName, const QString &indexName) Q_DECL_OVERRIDE;
};

} // namespace Server
} // namespace Akonadi

#endif // DBINTROSPECTOR_IMPL_H
