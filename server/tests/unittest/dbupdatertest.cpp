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

#include <QtCore/QDebug>
#include <QtCore/QVariant>
#include <QBuffer>

#include "storage/dbupdater.h"

QTEST_MAIN( DbUpdaterTest )

void DbUpdaterTest::testMysqlUpdateStatements()
{
  const QSqlDatabase db = QSqlDatabase::addDatabase( QLatin1String( "QMYSQL" ) );
  DbUpdater updater( db, QLatin1String( ":dbupdate.xml" ) );

  UpdateSet::Map updateSets;
  QVERIFY( updater.parseUpdateSets( 1, updateSets ) );
  QVERIFY( updateSets.contains( 2 ) );
  QVERIFY( updateSets.contains( 3 ) );
  QVERIFY( updateSets.contains( 4 ) );
  QVERIFY( updateSets.contains( 8 ) );
  QVERIFY( updateSets.contains( 10 ) );
  QVERIFY( updateSets.contains( 12 ) );
  QVERIFY( updateSets.contains( 13 ) );
  QVERIFY( updateSets.contains( 14 ) );
  QVERIFY( updateSets.contains( 15 ) );
  QVERIFY( updateSets.contains( 16 ) );
  QVERIFY( updateSets.contains( 17 ) );
  QVERIFY( updateSets.contains( 18 ) );
  QCOMPARE( updateSets.count(), 12 );

  updateSets.clear();
  QVERIFY( updater.parseUpdateSets( 13, updateSets ) );
  QVERIFY( !updateSets.contains( 2 ) );
  QVERIFY( !updateSets.contains( 3 ) );
  QVERIFY( !updateSets.contains( 4 ) );
  QVERIFY( !updateSets.contains( 8 ) );
  QVERIFY( !updateSets.contains( 10 ) );
  QVERIFY( !updateSets.contains( 12 ) );
  QVERIFY( !updateSets.contains( 13 ) );
  QVERIFY( updateSets.contains( 14 ) );
  QVERIFY( updateSets.contains( 15 ) );
  QVERIFY( updateSets.contains( 16 ) );
  QVERIFY( updateSets.contains( 17 ) );
  QVERIFY( updateSets.contains( 18 ) );
  QCOMPARE( updateSets.count(), 5 );

  QCOMPARE( updateSets.value( 14 ).statements.count(), 2 );
  QCOMPARE( updateSets.value( 16 ).statements.count(), 11 );
}

void DbUpdaterTest::testPsqlUpdateStatements()
{
  const QSqlDatabase db = QSqlDatabase::addDatabase( QLatin1String( "QPSQL" ) );
  DbUpdater updater( db, QLatin1String( ":dbupdate.xml" ) );

  UpdateSet::Map updateSets;
  QVERIFY( updater.parseUpdateSets( 1, updateSets ) );
  QVERIFY( updateSets.contains( 2 ) );
  QVERIFY( updateSets.contains( 3 ) );
  QVERIFY( updateSets.contains( 4 ) );
  QVERIFY( updateSets.contains( 8 ) );
  QVERIFY( updateSets.contains( 10 ) );
  QVERIFY( updateSets.contains( 12 ) );
  QVERIFY( updateSets.contains( 13 ) );
  QVERIFY( updateSets.contains( 14 ) );
  QVERIFY( updateSets.contains( 15 ) );
  QVERIFY( updateSets.contains( 16 ) );
  QVERIFY( updateSets.contains( 17 ) );
  QVERIFY( updateSets.contains( 18 ) );
  QCOMPARE( updateSets.count(), 12 );

  updateSets.clear();
  QVERIFY( updater.parseUpdateSets( 13, updateSets ) );
  QVERIFY( !updateSets.contains( 2 ) );
  QVERIFY( !updateSets.contains( 3 ) );
  QVERIFY( !updateSets.contains( 4 ) );
  QVERIFY( !updateSets.contains( 8 ) );
  QVERIFY( !updateSets.contains( 10 ) );
  QVERIFY( !updateSets.contains( 12 ) );
  QVERIFY( !updateSets.contains( 13 ) );
  QVERIFY( updateSets.contains( 14 ) );
  QVERIFY( updateSets.contains( 15 ) );
  QVERIFY( updateSets.contains( 16 ) );
  QVERIFY( updateSets.contains( 17 ) );
  QVERIFY( updateSets.contains( 18 ) );
  QCOMPARE( updateSets.count(), 5 );

  QCOMPARE( updateSets.value( 14 ).statements.count(), 2 );
  QCOMPARE( updateSets.value( 16 ).statements.count(), 11 );
  QCOMPARE( updateSets.value( 17 ).statements.count(), 0 );
}

#include "dbupdatertest.moc"
