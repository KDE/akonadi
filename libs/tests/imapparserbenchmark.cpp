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
};

#include "imapparserbenchmark.moc"

QTEST_MAIN( ImapParserBenchmark )
