/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    QVector<ForeignKey> foreignKeyConstraints(const QString &tableName) override;
    QString hasIndexQuery(const QString &tableName, const QString &indexName) override;
};

class DbIntrospectorSqlite : public DbIntrospector
{
public:
    explicit DbIntrospectorSqlite(const QSqlDatabase &database);
    QVector<ForeignKey> foreignKeyConstraints(const QString &tableName) override;
    QString hasIndexQuery(const QString &tableName, const QString &indexName) override;
};

class DbIntrospectorPostgreSql : public DbIntrospector
{
public:
    explicit DbIntrospectorPostgreSql(const QSqlDatabase &database);
    QVector<ForeignKey> foreignKeyConstraints(const QString &tableName) override;
    QString hasIndexQuery(const QString &tableName, const QString &indexName) override;
};

} // namespace Server
} // namespace Akonadi

#endif // DBINTROSPECTOR_IMPL_H
