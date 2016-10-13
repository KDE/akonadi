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

#include <QDebug>
#include <QVariant>
#include <QBuffer>
#include <QDir>

#include "storage/dbupdater.h"

using namespace Akonadi::Server;

void DbUpdaterTest::initTestCase()
{
    Q_INIT_RESOURCE(akonadidb);
}

void DbUpdaterTest::testMysqlUpdateStatements()
{
    const QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"));
    DbUpdater updater(db, QStringLiteral(":unittest_dbupdate.xml"));

    UpdateSet::Map updateSets;
    QVERIFY(updater.parseUpdateSets(1, updateSets));
    QVERIFY(updateSets.contains(2));
    QVERIFY(updateSets.contains(3));
    QVERIFY(updateSets.contains(4));
    QVERIFY(updateSets.contains(8));
    QVERIFY(updateSets.contains(10));
    QVERIFY(updateSets.contains(12));
    QVERIFY(updateSets.contains(13));
    QVERIFY(updateSets.contains(14));
    QVERIFY(updateSets.contains(15));
    QVERIFY(updateSets.contains(16));
    QVERIFY(updateSets.contains(17));
    QVERIFY(updateSets.contains(18));
    QVERIFY(updateSets.contains(19));
    QVERIFY(updateSets.contains(22));
    QCOMPARE(updateSets.count(), 16);

    updateSets.clear();
    QVERIFY(updater.parseUpdateSets(13, updateSets));
    QVERIFY(!updateSets.contains(2));
    QVERIFY(!updateSets.contains(3));
    QVERIFY(!updateSets.contains(4));
    QVERIFY(!updateSets.contains(8));
    QVERIFY(!updateSets.contains(10));
    QVERIFY(!updateSets.contains(12));
    QVERIFY(!updateSets.contains(13));
    QVERIFY(updateSets.contains(14));
    QVERIFY(updateSets.contains(15));
    QVERIFY(updateSets.contains(16));
    QVERIFY(updateSets.contains(17));
    QVERIFY(updateSets.contains(18));
    QVERIFY(updateSets.contains(19));
    QVERIFY(updateSets.contains(22));
    QCOMPARE(updateSets.count(), 9);

    QCOMPARE(updateSets.value(14).statements.count(), 2);
    QCOMPARE(updateSets.value(16).statements.count(), 11);
    QCOMPARE(updateSets.value(22).statements.count(), 6);
}

void DbUpdaterTest::testPsqlUpdateStatements()
{
    const QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
    DbUpdater updater(db, QStringLiteral(":unittest_dbupdate.xml"));

    UpdateSet::Map updateSets;
    QVERIFY(updater.parseUpdateSets(1, updateSets));
    QVERIFY(updateSets.contains(2));
    QVERIFY(updateSets.contains(3));
    QVERIFY(updateSets.contains(4));
    QVERIFY(updateSets.contains(8));
    QVERIFY(updateSets.contains(10));
    QVERIFY(updateSets.contains(12));
    QVERIFY(updateSets.contains(13));
    QVERIFY(updateSets.contains(14));
    QVERIFY(updateSets.contains(15));
    QVERIFY(updateSets.contains(16));
    QVERIFY(updateSets.contains(17));
    QVERIFY(updateSets.contains(18));
    QVERIFY(updateSets.contains(19));
    QCOMPARE(updateSets.count(), 16);

    updateSets.clear();
    QVERIFY(updater.parseUpdateSets(13, updateSets));
    QVERIFY(!updateSets.contains(2));
    QVERIFY(!updateSets.contains(3));
    QVERIFY(!updateSets.contains(4));
    QVERIFY(!updateSets.contains(8));
    QVERIFY(!updateSets.contains(10));
    QVERIFY(!updateSets.contains(12));
    QVERIFY(!updateSets.contains(13));
    QVERIFY(updateSets.contains(14));
    QVERIFY(updateSets.contains(15));
    QVERIFY(updateSets.contains(16));
    QVERIFY(updateSets.contains(17));
    QVERIFY(updateSets.contains(18));
    QVERIFY(updateSets.contains(19));
    QCOMPARE(updateSets.count(), 9);

    QCOMPARE(updateSets.value(14).statements.count(), 2);
    QCOMPARE(updateSets.value(16).statements.count(), 11);
    QCOMPARE(updateSets.value(17).statements.count(), 0);
    QCOMPARE(updateSets.value(22).statements.count(), 0);
}

void DbUpdaterTest::cleanupTestCase()
{
    Q_CLEANUP_RESOURCE(akonadidb);
}

QTEST_MAIN(DbUpdaterTest)
