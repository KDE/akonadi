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
#include <private/imapparser_p.h>

using namespace Akonadi;

Q_DECLARE_METATYPE(QList<QByteArray>)

class ImapParserBenchmark : public QObject
{
  Q_OBJECT
  private:
    void geneateParseParenthesizedListData()
    {
      QTest::addColumn<QByteArray>( "data" );
      QTest::newRow( "empty" ) << QByteArray();
      QTest::newRow( "unnested" ) << QByteArray("(\"Foo Bar\" NIL \"foobar\" \"test.com\")");
      QTest::newRow( "nested" ) << QByteArray("((\"Foo Bar\" NIL \"foobar\" \"test.com\"))");
      QTest::newRow( "nested-long" ) << QByteArray("(UID 86 REV 0 MIMETYPE \"message/rfc822\" COLLECTIONID 13 SIZE 6114 FLAGS (\\SEEN)"
                                                   " ANCESTORS ((13 \"/INBOX\") (12 \"imap://mail@mail.test.com/\") (0 \"\")) PLD:ENVELOPE[1] {396}"
                                                   " (\"Fri, 04 Jun 2010 09:07:54 +0200\" \"Re: [ADMIN] foobar available again!\""
                                                   " ((\"Foo Bar\" NIL \"foobar\" \"test.com\"))"
                                                   " NIL NIL"
                                                   " ((\"Asdf Bla Blub\" NIL \"asdf.bla.blub\" \"123test.org\"))"
                                                   " ((NIL NIL \"muh.kuh\" \"lalala.com\") (\"Konqi KDE\" NIL \"konqi\" \"kde.org\") (NIL NIL \"all\" \"test.com\"))"
                                                   " NIL \"<201006040905.33367.foo.bar@test.com>\" \"<4C08A64A.9020205@123test.org>\""
                                                   " \"<201006040142.56540.muh.kuh@lalala.com> <201006040704.39648.konqi@kde.org> <201006040905.33367.foo.bar@test.com>\""
                                                   "))");
    }

  private Q_SLOTS:
    void quote_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("10-idle") << QByteArray("ababababab");
        QTest::newRow("10-quote") << QByteArray("\"abababab\"");
        QTest::newRow("50-idle") << QByteArray("ababababababababababababababababababababababababab");
        QTest::newRow("50-quote") << QByteArray("\"abababab\ncabababab\ncabababab\ncabababab\ncabababab\"");
    }

    void quote()
    {
        QFETCH(QByteArray, input);
        QBENCHMARK {
            ImapParser::quote(input);
        }
    }

    void join_data()
    {
        QTest::addColumn<QList<QByteArray> >("list");
        QTest::newRow("empty") << QList<QByteArray>();
        QTest::newRow("single") << (QList<QByteArray>() << "ababab");
        QTest::newRow("two") << (QList<QByteArray>() << "ababab" << "ababab");
        QTest::newRow("five") << (QList<QByteArray>() << "ababab" << "ababab" << "ababab" << "ababab" << "ababab");
        QList<QByteArray> list;
        for (int i = 0; i < 50; ++i) {
            list << "ababab";
        }
        QTest::newRow("a lot") << list;
    }

    void join()
    {
        QFETCH(QList<QByteArray>, list);
        QBENCHMARK {
            ImapParser::join(list, " ");
        }
    }

    void parseParenthesizedQVarLengthArray_data()
    {
        geneateParseParenthesizedListData();
    }

    void parseParenthesizedQVarLengthArray()
    {
        QFETCH(QByteArray, data);
        QVarLengthArray<QByteArray, 16> result;
        QBENCHMARK {
            ImapParser::parseParenthesizedList(data, result, 0);
        }
    }

    void parseParenthesizedQList_data()
    {
      geneateParseParenthesizedListData();
    }

    void parseParenthesizedQList()
    {
      QFETCH( QByteArray, data );
      QList<QByteArray> result;
      QBENCHMARK {
        ImapParser::parseParenthesizedList( data, result, 0 );
      }
    }

    void parseString_data()
    {
      QTest::addColumn<QByteArray>( "data" );
      QTest::newRow("plain") << QByteArray("fooobarasdf something more lalala");
      QTest::newRow("quoted") << QByteArray("\"fooobarasdf\" something more lalala");
    }

    void parseString()
    {
      QFETCH(QByteArray, data);
      QByteArray result;
      qint64 sum = 0;
      QBENCHMARK {
        sum += ImapParser::parseString( data, result );
      }
      QVERIFY(!result.isEmpty());
      QVERIFY(sum > 0);
    }

    void parseNumber()
    {
      QByteArray data( "123456" );
      qint64 result;
      bool ok = false;
      QBENCHMARK {
        ImapParser::parseNumber( data, result, &ok );
      }
      QVERIFY(ok);
      QCOMPARE(result, qint64(123456));
    }

    void parseDateTime()
    {
      QByteArray data( "28-May-2006 01:03:35 +0000" );
      QDateTime result;
      QBENCHMARK {
        ImapParser::parseDateTime( data, result );
      }
      QCOMPARE(result.toString( QString::fromUtf8( "dd-MMM-yyyy hh:mm:ss +0000" ) ), QString::fromUtf8( data ));
    }
};

#include "imapparserbenchmark.moc"

QTEST_APPLESS_MAIN(ImapParserBenchmark)
