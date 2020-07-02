/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "../sharedvaluepool_p.h"
#include <QTest>

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
        QTest::addColumn<int>("size");
        QTest::newRow("10") << 10;
        QTest::newRow("100") << 100;
    }

    void testQVector()
    {
        QFETCH(int, size);
        QVector<QByteArray> data;
        Internal::SharedValuePool<QByteArray, QVector> pool;

        for (int i = 0; i < size; ++i) {
            QByteArray b(10, static_cast<char>(i));
            data.push_back(b);
            QCOMPARE(pool.sharedValue(b), b);
            QCOMPARE(pool.sharedValue(b), b);
        }

        QBENCHMARK {
            foreach (const QByteArray &b, data)
            {
                pool.sharedValue(b);
            }
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

QTEST_MAIN(SharedValuePoolTest)

#include "sharedvaluepooltest.moc"
