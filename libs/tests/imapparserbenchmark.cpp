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

#include <QtTest/QTest>
#include "../imapparser_p.h"

using namespace Akonadi;

Q_DECLARE_METATYPE( QList<QByteArray> )

class ImapParserBenchmark : public QObject
{
  Q_OBJECT
  private slots:
    void quote_data()
    {
      QTest::addColumn<QByteArray>( "input" );
      QTest::newRow( "empty" ) << QByteArray();
      QTest::newRow( "10-idle" ) << QByteArray( "ababababab" );
      QTest::newRow( "10-quote" ) << QByteArray( "\"abababab\"" );
      QTest::newRow( "50-idle" ) << QByteArray(  "ababababababababababababababababababababababababab" );
      QTest::newRow( "50-quote" ) << QByteArray( "\"abababab\ncabababab\ncabababab\ncabababab\ncabababab\"" );
    }

    void quote()
    {
      QFETCH( QByteArray, input );
      QBENCHMARK {
        ImapParser::quote( input );
      }
    }

    void join_data()
    {
      QTest::addColumn<QList<QByteArray> >( "list" );
      QTest::newRow( "empty" ) << QList<QByteArray>();
      QTest::newRow( "single" ) << (QList<QByteArray>() << "ababab");
      QTest::newRow( "two" ) << (QList<QByteArray>() << "ababab" << "ababab");
      QTest::newRow( "five" ) << (QList<QByteArray>() << "ababab" << "ababab" << "ababab" << "ababab" << "ababab");
      QList<QByteArray> list;
      for ( int i = 0; i < 50; ++i )
        list << "ababab";
      QTest::newRow( "a lot" ) << list;
    }

    void join()
    {
      QFETCH( QList<QByteArray>, list );
      QBENCHMARK {
        ImapParser::join( list, " " );
      }
    }
};

#include "imapparserbenchmark.moc"

QTEST_APPLESS_MAIN( ImapParserBenchmark )
