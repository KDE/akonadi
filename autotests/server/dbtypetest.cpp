/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <aktest.h>
#include <storage/dbtype.h>

#define QL1S(x) QLatin1StringView(x)

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(DbType::Type)

class DbTypeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDriverName_data()
    {
        QTest::addColumn<QString>("driverName");
        QTest::addColumn<DbType::Type>("dbType");

        QTest::newRow("mysql") << "QMYSQL" << DbType::MySQL;
        QTest::newRow("sqlite") << "QSQLITE" << DbType::Sqlite;
        QTest::newRow("psql") << "QPSQL" << DbType::PostgreSQL;
    }

    void testDriverName()
    {
        QFETCH(QString, driverName);
        QFETCH(DbType::Type, dbType);

        QCOMPARE(DbType::typeForDriverName(driverName), dbType);

        if (QSqlDatabase::drivers().contains(driverName)) {
            QSqlDatabase db = QSqlDatabase::addDatabase(driverName, driverName);
            QCOMPARE(DbType::type(db), dbType);
        }
    }
};

AKTEST_MAIN(DbTypeTest)

#include "dbtypetest.moc"
