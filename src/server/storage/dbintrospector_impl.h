/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "dbintrospector.h"

namespace Akonadi
{
namespace Server
{
class DbIntrospectorMySql : public DbIntrospector
{
public:
    explicit DbIntrospectorMySql(const QSqlDatabase &database);
    QList<ForeignKey> foreignKeyConstraints(const QString &tableName) override;
    QString hasIndexQuery(const QString &tableName, const QString &indexName) override;
    QString getAutoIncrementValueQuery(const QString &tableName, const QString &idColumn) override;
    QString updateAutoIncrementValueQuery(const QString &tableName, const QString &idColumn, qint64 value) override;
};

class DbIntrospectorSqlite : public DbIntrospector
{
public:
    explicit DbIntrospectorSqlite(const QSqlDatabase &database);
    QList<ForeignKey> foreignKeyConstraints(const QString &tableName) override;
    QString hasIndexQuery(const QString &tableName, const QString &indexName) override;
    QString getAutoIncrementValueQuery(const QString &tableName, const QString &idColumn) override;
    QString updateAutoIncrementValueQuery(const QString &tableName, const QString &idColumn, qint64 value) override;
};

class DbIntrospectorPostgreSql : public DbIntrospector
{
public:
    explicit DbIntrospectorPostgreSql(const QSqlDatabase &database);
    QList<ForeignKey> foreignKeyConstraints(const QString &tableName) override;
    QString hasIndexQuery(const QString &tableName, const QString &indexName) override;
    QString getAutoIncrementValueQuery(const QString &tableName, const QString &idColumn) override;
    QString updateAutoIncrementValueQuery(const QString &tableName, const QString &idColumn, qint64 value) override;
};

} // namespace Server
} // namespace Akonadi
