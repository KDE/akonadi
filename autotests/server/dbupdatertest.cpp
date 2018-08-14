/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "dbupdatertest.h"


#include "storage/dbupdater.h"
#include <QTest>

using namespace Akonadi::Server;

void DbUpdaterTest::initTestCase()
{
    Q_INIT_RESOURCE(akonadidb);
}

void DbUpdaterTest::testMysqlUpdateStatements()
{
    const QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"));
    DbUpdater updater(db, QStringLiteral(":unittest_dbupdate.xml"));

    {
        UpdateSet::Map updateSets;
        QVERIFY(updater.parseUpdateSets(1, updateSets));
        const auto expectedSets ={ 2, 3, 4, 8, 10, 12, 13, 14, 15 ,16, 17, 18, 19, 20, 21, 22, 23,  25, 26, 28, 31, 35 };
        for (const auto expected : expectedSets) {
            QVERIFY(updateSets.contains(expected));
        }
        QCOMPARE(std::size_t(updateSets.count()), expectedSets.size());
    }

    {
        UpdateSet::Map updateSets;
        QVERIFY(updater.parseUpdateSets(13, updateSets));
        const auto expectedSets = { 14, 15 ,16, 17, 18, 19, 20, 21, 22, 23, 25, 26, 28, 31, 35 };
        for (int i = 0; i < 13; ++i) {
            QVERIFY(!updateSets.contains(i));
        }
        for (const auto expected : expectedSets) {
            QVERIFY(updateSets.contains(expected));
        }
        QCOMPARE(std::size_t(updateSets.count()), expectedSets.size());

        QCOMPARE(updateSets.value(14).statements.count(), 2);
        QCOMPARE(updateSets.value(16).statements.count(), 11);
        QCOMPARE(updateSets.value(22).statements.count(), 6);
    }
}

void DbUpdaterTest::testPsqlUpdateStatements()
{
    const QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
    DbUpdater updater(db, QStringLiteral(":unittest_dbupdate.xml"));

    {
        UpdateSet::Map updateSets;
        QVERIFY(updater.parseUpdateSets(1, updateSets));
        const auto expectedSets = { 2, 3, 4, 8, 10, 12, 13, 14, 15, 16, 19, 23, 24, 25, 26, 28, 30, 33, 35 };
        for (const auto expected : expectedSets) {
            QVERIFY(updateSets.contains(expected));
        }
        QCOMPARE(updateSets.count(), expectedSets.size());
    }

    {
        UpdateSet::Map updateSets;
        QVERIFY(updater.parseUpdateSets(13, updateSets));
        const auto expectedSets = { 14, 15 ,16, 19, 23, 24, 25, 26, 28, 30, 33, 35 };
        for (int i = 0; i < 13; ++i) {
            QVERIFY(!updateSets.contains(i));
        }
        for (const auto expected : expectedSets) {
            QVERIFY(updateSets.contains(expected));
        }
        QCOMPARE(updateSets.count(), expectedSets.size());

        QCOMPARE(updateSets.value(14).statements.count(), 2);
        QCOMPARE(updateSets.value(16).statements.count(), 11);
        QCOMPARE(updateSets.value(17).statements.count(), 0);
        QCOMPARE(updateSets.value(22).statements.count(), 0);
    }
}

void DbUpdaterTest::cleanupTestCase()
{
    Q_CLEANUP_RESOURCE(akonadidb);
}

QTEST_MAIN(DbUpdaterTest)
