/****************************************************************************
**
** SPDX-FileCopyrightText: 2009 Nokia Corporation and /or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** SPDX-FileCopyrightText: LGPL-2.1-only WITH Qt-LGPL-exception-1.1 OR GPL-3.0-only
**
****************************************************************************/

#pragma once

#include <QtSql/private/qsqlcachedresult_p.h>
#include <QtSql/qsqldriver.h>
#include <QtSql/qsqlresult.h>

struct sqlite3;

QT_BEGIN_NAMESPACE
class QSQLiteDriverPrivate;
class QSQLiteResultPrivate;
class QSQLiteDriver;

class QSQLiteDriver : public QSqlDriver
{
    Q_OBJECT
    friend class QSQLiteResult;

public:
    explicit QSQLiteDriver(QObject *parent = nullptr);
    explicit QSQLiteDriver(sqlite3 *connection, QObject *parent = nullptr);
    ~QSQLiteDriver() override;
    bool hasFeature(DriverFeature f) const override;
    bool open(const QString &db, const QString &user, const QString &password, const QString &host, int port, const QString &connOpts) override;
    void close() override;
    QSqlResult *createResult() const override;
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;
    QStringList tables(QSql::TableType) const override;

    QSqlRecord record(const QString &tablename) const override;
    QSqlIndex primaryIndex(const QString &table) const override;
    QVariant handle() const override;
    QString escapeIdentifier(const QString &identifier, IdentifierType) const override;

private:
    Q_DECLARE_PRIVATE(QSQLiteDriver)
};

QT_END_NAMESPACE
