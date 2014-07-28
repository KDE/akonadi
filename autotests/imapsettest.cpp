/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "imapsettest.h"
#include <private/imapset_p.h>

#include <qtest.h>

QTEST_MAIN( ImapSetTest )

Q_DECLARE_METATYPE( QList<qint64> )

using namespace Akonadi;

void ImapSetTest::testAddList_data()
{
  QTest::addColumn<QList<qint64> >( "source" );
  QTest::addColumn<ImapInterval::List>( "intervals" );
  QTest::addColumn<QByteArray>( "seqset" );

  // empty set
  QList<qint64> source;
  ImapInterval::List intervals;
  QTest::newRow( "empty" ) << source << intervals << QByteArray();

  // single value
  source << 4;
  intervals << ImapInterval( 4, 4 );
  QTest::newRow( "single value" ) << source << intervals << QByteArray( "4" );

  // single 2-value interval
  source << 5;
  intervals.clear();
  intervals << ImapInterval( 4, 5 );
  QTest::newRow( "single 2 interval" ) << source << intervals << QByteArray( "4:5" );

  // single large interval
  source << 6 << 3 << 7 << 2  << 8;
  intervals.clear();
  intervals << ImapInterval( 2, 8 );
  QTest::newRow( "single 7 interval" ) << source << intervals << QByteArray( "2:8" );

  // double interval
  source << 12;
  intervals << ImapInterval( 12, 12 );
  QTest::newRow( "double interval" ) << source << intervals << QByteArray( "2:8,12" );
}

void ImapSetTest::testAddList()
{
  QFETCH( QList<qint64>, source );
  QFETCH( ImapInterval::List, intervals );
  QFETCH( QByteArray, seqset );

  ImapSet set;
  set.add( source );
  ImapInterval::List result = set.intervals();
  QCOMPARE( result, intervals );
  QCOMPARE( set.toImapSequenceSet(), seqset );
}
