/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "../sharedvaluepool_p.h"
#include <qtest.h>

#include <QVector>
#include <QSet>
#include <set>
#include <vector>

using namespace Akonadi;

class SharedValuePoolTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void testQVector_data()
    {
      QTest::addColumn<int>( "size" );
      QTest::newRow( "10" ) << 10;
      QTest::newRow( "100" ) << 100;
    }

    void testQVector()
    {
      QFETCH( int, size );
      QVector<QByteArray> data;
      Internal::SharedValuePool<QByteArray, QVector> pool;

      for ( int i = 0; i < size; ++i ) {
        QByteArray b( 10, (char)i );
        data.push_back( b );
        QCOMPARE( pool.sharedValue( b ), b );
        QCOMPARE( pool.sharedValue( b ), b );
      }

      QBENCHMARK {
        foreach ( const QByteArray &b, data )
          pool.sharedValue( b );
      }
    }

    /*void testQSet_data()
    {
      QTest::addColumn<int>( "size" );
      QTest::newRow( "10" ) << 10;
      QTest::newRow( "100" ) << 100;
    }

    void testQSet()
    {
      QFETCH( int, size );
      QVector<QByteArray> data;
      Internal::SharedValuePool<QByteArray, QSet> pool;

      for ( int i = 0; i < size; ++i ) {
        QByteArray b( 10, (char)i );
        data.push_back( b );
        QCOMPARE( pool.sharedValue( b ), b );
        QCOMPARE( pool.sharedValue( b ), b );
      }

      QBENCHMARK {
        foreach ( const QByteArray &b, data )
          pool.sharedValue( b );
      }
    }*/
};

QTEST_MAIN( SharedValuePoolTest )

#include "sharedvaluepooltest.moc"
