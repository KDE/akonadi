/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QTest>
#include <QDebug>

#include <aktest.h>
#include <storage/dbintrospector.h>
#include <storage/dbexception.h>

#define QL1S(x) QLatin1String(x)

using namespace Akonadi::Server;

class DbIntrospectorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testHasIndexQuery_data()
    {
        QTest::addColumn<QString>("driverName");
        QTest::addColumn<QString>("indexQuery");

        QTest::newRow("mysql") << "QMYSQL" << "SHOW INDEXES FROM myTable WHERE `Key_name` = 'myIndex'";
        QTest::newRow("sqlite") << "QSQLITE" << "SELECT * FROM sqlite_master WHERE type='index' AND tbl_name='myTable' AND name='myIndex';";
        QTest::newRow("psql") << "QPSQL" << "SELECT indexname FROM pg_catalog.pg_indexes WHERE tablename ilike 'myTable' AND  indexname ilike 'myIndex' UNION SELECT conname FROM pg_catalog.pg_constraint  WHERE conname ilike 'myIndex'";
    }

    void testHasIndexQuery()
    {
        QFETCH(QString, driverName);
        QFETCH(QString, indexQuery);

        if (QSqlDatabase::drivers().contains(driverName)) {
            QSqlDatabase db = QSqlDatabase::addDatabase(driverName, driverName);
            DbIntrospector::Ptr introspector = DbIntrospector::createInstance(db);
            QVERIFY(introspector);
            QCOMPARE(introspector->hasIndexQuery(QL1S("myTable"), QL1S("myIndex")), indexQuery);
        }
    }

    void testHasIndex_data()
    {
        QTest::addColumn<QString>("driverName");
        QTest::addColumn<bool>("shouldThrow");

        QTest::newRow("mysql") << "QMYSQL" << true;
        QTest::newRow("sqlite") << "QSQLITE" << true;
        QTest::newRow("psql") << "QPSQL" << true;
    }

    void testHasIndex()
    {
        QFETCH(QString, driverName);
        QFETCH(bool, shouldThrow);

        if (QSqlDatabase::drivers().contains(driverName)) {
            QSqlDatabase db = QSqlDatabase::addDatabase(driverName, driverName + QL1S("hasIndex"));
            DbIntrospector::Ptr introspector = DbIntrospector::createInstance(db);
            QVERIFY(introspector);

            bool didThrow = false;
            try {
                QVERIFY(introspector->hasIndex(QL1S("myTable"), QL1S("myIndex")));
            } catch (const DbException &e) {
                didThrow = true;
                qDebug() << Q_FUNC_INFO << e.what();
            }
            QCOMPARE(didThrow, shouldThrow);
        }
    }

};

AKTEST_MAIN(DbIntrospectorTest)

#include "dbintrospectortest.moc"
