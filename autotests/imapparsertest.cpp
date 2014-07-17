/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "imapparsertest.h"
#include <akonadi/private/imapparser_p.h>
#include <qtest_kde.h>

#include <QtCore/QDebug>
#include <QtCore/QVariant>

Q_DECLARE_METATYPE( QList<QByteArray> )
Q_DECLARE_METATYPE( QList<int> )

using namespace Akonadi;

QTEST_KDEMAIN( ImapParserTest, NoGUI )

void ImapParserTest::testStripLeadingSpaces( )
{
  QByteArray input = "  a  ";
  int result;

  // simple leading spaces at the beginning
  result = ImapParser::stripLeadingSpaces( input, 0 );
  QCOMPARE( result, 2 );

  // simple leading spaces in the middle
  result = ImapParser::stripLeadingSpaces( input, 1 );
  QCOMPARE( result, 2 );

  // no leading spaces
  result = ImapParser::stripLeadingSpaces( input, 2 );
  QCOMPARE( result, 2 );

  // trailing spaces
  result = ImapParser::stripLeadingSpaces( input, 3 );
  QCOMPARE( result, 5 );

  // out of bounds access
  result = ImapParser::stripLeadingSpaces( input, input.length() );
  QCOMPARE( result, input.length() );
}

void ImapParserTest::testParseQuotedString( )
{
  QByteArray input = "\"quoted \\\"NIL\\\"   string inside\"";
  QByteArray result;
  int consumed;

  // the whole thing
  consumed = ImapParser::parseQuotedString( input, result, 0 );
  QCOMPARE( result, QByteArray( "quoted \"NIL\"   string inside" ) );
  QCOMPARE( consumed, 32 );

  // unqoted string
  consumed = ImapParser::parseQuotedString( input, result, 1 );
  QCOMPARE( result, QByteArray( "quoted" ) );
  QCOMPARE( consumed, 7 );

  // whitespaces in qouted string
  consumed = ImapParser::parseQuotedString( input, result, 14 );
  QCOMPARE( result, QByteArray( "   string inside" ) );
  QCOMPARE( consumed, 32 );

  // whitespaces before unquoted string
  consumed = ImapParser::parseQuotedString( input, result, 15 );
  QCOMPARE( result, QByteArray( "string" ) );
  QCOMPARE( consumed, 24 );

  // NIL and emptyness tests
  input = "NIL \"NIL\" \"\"";

  // unqoted NIL
  consumed = ImapParser::parseQuotedString( input, result, 0 );
  QVERIFY( result.isNull() );
  QCOMPARE( consumed, 3 );

  // quoted NIL
  consumed = ImapParser::parseQuotedString( input, result, 3 );
  QCOMPARE( result, QByteArray( "NIL" ) );
  QCOMPARE( consumed, 9 );

  // quoted empty string
  consumed = ImapParser::parseQuotedString( input, result, 9 );
  QCOMPARE( result, QByteArray( "" ) );
  QCOMPARE( consumed, 12 );

  // unquoted string at input end
  input = "some string";
  consumed = ImapParser::parseQuotedString( input, result, 4 );
  QCOMPARE( result, QByteArray( "string" ) );
  QCOMPARE( consumed, 11 );

  // out of bounds access
  consumed = ImapParser::parseQuotedString( input, result, input.length() );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, input.length() );

  // de-quoting
  input = "\"\\\"some \\\\ quoted stuff\\\"\"";
  consumed = ImapParser::parseQuotedString( input, result, 0 );
  QCOMPARE( result, QByteArray( "\"some \\ quoted stuff\"" ) );
  QCOMPARE( consumed, input.length() );

  // linebreak as separator
  input = "LOGOUT\nFOO";
  consumed = ImapParser::parseQuotedString( input, result, 0 );
  QCOMPARE( result, QByteArray("LOGOUT") );
  QCOMPARE( consumed, 6 );

}

void ImapParserTest::testParseString( )
{
  QByteArray input = "\"quoted\" unquoted {7}\nliteral {0}\n empty literal";
  QByteArray result;
  int consumed;

  // quoted strings
  consumed = ImapParser::parseString( input, result, 0 );
  QCOMPARE( result, QByteArray( "quoted" ) );
  QCOMPARE( consumed, 8 );

  // unquoted string
  consumed = ImapParser::parseString( input, result, 8 );
  QCOMPARE( result, QByteArray( "unquoted" ) );
  QCOMPARE( consumed, 17 );

  // literal string
  consumed = ImapParser::parseString( input, result, 17 );
  QCOMPARE( result, QByteArray( "literal" ) );
  QCOMPARE( consumed, 29 );

  // empty literal string
  consumed = ImapParser::parseString( input, result, 29 );
  QCOMPARE( result, QByteArray( "" ) );
  QCOMPARE( consumed, 34 );

  // out of bounds access
  consumed = ImapParser::parseString( input, result, input.length() );
  QCOMPARE( result, QByteArray() );
  QCOMPARE( consumed, input.length() );
}

void ImapParserTest::testParseParenthesizedList_data()
{
  QTest::addColumn<QByteArray>( "input" );
  QTest::addColumn<QList<QByteArray> >( "result" );
  QTest::addColumn<int>( "consumed" );

  QList<QByteArray> reference;

  QTest::newRow( "null" ) << QByteArray() << reference << 0;
  QTest::newRow( "empty" ) << QByteArray( "()" ) << reference << 2;
  QTest::newRow( "empty with space" ) << QByteArray( " ( )" ) << reference<< 4;
  QTest::newRow( "no list" ) << QByteArray( "some list-less text" ) << reference << 0;
  QTest::newRow( "\n" ) << QByteArray() << reference << 0;

  reference << "entry1";
  reference << "entry2()";
  reference << "(sub list)";
  reference << ")))";
  reference << "entry3";
  QTest::newRow( "complex" ) << QByteArray( "(entry1 \"entry2()\" (sub list) \")))\" {6}\nentry3) end" ) << reference << 47;

  reference.clear();
  reference << "foo";
  reference << "\n\nbar\n";
  reference << "bla";
  QTest::newRow( "newline literal" ) << QByteArray("(foo {6}\n\n\nbar\n bla)") << reference << 20;

  reference.clear();
  reference << "partid";
  reference << "body";
  QTest::newRow( "CRLF literal separator" ) << QByteArray( "(partid {4}\r\nbody)") << reference << 18;

  reference.clear();
  reference << "partid";
  reference << "\n\rbody\n\r";
  QTest::newRow( "CRLF literal separator 2" ) << QByteArray( "(partid {8}\r\n\n\rbody\n\r)") << reference << 22;

  reference.clear();
  reference << "NAME";
  reference << "net)";
  QTest::newRow( "spurious newline" ) << QByteArray("(NAME \"net)\"\n)") << reference << 14;

  reference.clear();
  reference << "(42 \"net)\")";
  reference << "(0 \"\")";
  QTest::newRow( "list of lists" ) << QByteArray( "((42 \"net)\") (0 \"\"))" ) << reference << 20;
}

void ImapParserTest::testParseParenthesizedList( )
{
  QFETCH( QByteArray, input );
  QFETCH( QList<QByteArray>, result );
  QFETCH( int, consumed );

  QList<QByteArray> realResult;

  int reallyConsumed = ImapParser::parseParenthesizedList( input, realResult, 0 );
  QCOMPARE( realResult, result );
  QCOMPARE( reallyConsumed, consumed );

  // briefly also test the other overload
  QVarLengthArray<QByteArray,16> realVLAResult;
  reallyConsumed = ImapParser::parseParenthesizedList( input, realVLAResult, 0 );
  QCOMPARE( reallyConsumed, consumed );

  // newline literal (based on itemappendtest bug)
  input = "(foo {6}\n\n\nbar\n bla)";
  consumed = ImapParser::parseParenthesizedList( input, result );
}

void ImapParserTest::testParseNumber()
{
  QByteArray input = " 1a23.4";
  qint64 result;
  int pos;
  bool ok;

  // empty string
  pos = ImapParser::parseNumber( QByteArray(), result, &ok );
  QCOMPARE( ok, false );
  QCOMPARE( pos, 0 );

  // leading spaces at the beginning
  pos = ImapParser::parseNumber( input, result, &ok, 0 );
  QCOMPARE( ok, true );
  QCOMPARE( pos, 2 );
  QCOMPARE( result, 1ll );

  // multiple digits
  pos = ImapParser::parseNumber( input, result, &ok, 3 );
  QCOMPARE( ok, true );
  QCOMPARE( pos, 5 );
  QCOMPARE( result, 23ll );

  // number at input end
  pos = ImapParser::parseNumber( input, result, &ok, 6 );
  QCOMPARE( ok, true );
  QCOMPARE( pos, 7 );
  QCOMPARE( result, 4ll );

  // out of bounds access
  pos = ImapParser::parseNumber( input, result, &ok, input.length() );
  QCOMPARE( ok, false );
  QCOMPARE( pos, input.length() );
}

void ImapParserTest::testQuote_data()
{
  QTest::addColumn<QByteArray>( "unquoted" );
  QTest::addColumn<QByteArray>( "quoted" );

  QTest::newRow( "empty" ) << QByteArray("") << QByteArray("\"\"");
  QTest::newRow( "simple" ) << QByteArray("bla") << QByteArray("\"bla\"");
  QTest::newRow( "double-quotes" ) << QByteArray("\"test\"test\"") << QByteArray("\"\\\"test\\\"test\\\"\"");
  QTest::newRow( "backslash" ) << QByteArray("\\") << QByteArray("\"\\\\\"");
  QByteArray binaryNonEncoded;
  binaryNonEncoded += '\000';
  QByteArray binaryEncoded("\"" );
  binaryEncoded += '\000';
  binaryEncoded += '"';
  QTest::newRow( "binary" ) << binaryNonEncoded << binaryEncoded;

  QTest::newRow( "LF" ) << QByteArray( "\n" ) << QByteArray( "\"\\n\"" );
  QTest::newRow( "CR" ) << QByteArray( "\r" ) << QByteArray( "\"\\r\"" );
  QTest::newRow( "double quote" ) << QByteArray( "\"" ) << QByteArray( "\"\\\"\"" );
  QTest::newRow( "mixed 1" ) << QByteArray( "a\nb\\c" ) << QByteArray( "\"a\\nb\\\\c\"" );
  QTest::newRow( "mixed 2" ) << QByteArray( "\"a\rb\"" ) << QByteArray( "\"\\\"a\\rb\\\"\"" );
}

void ImapParserTest::testQuote()
{
  QFETCH( QByteArray, unquoted );
  QFETCH( QByteArray, quoted );
  QCOMPARE( ImapParser::quote( unquoted ), quoted );
}

void ImapParserTest::testMessageParser_data()
{
  QTest::addColumn<QList<QByteArray> >( "input" );
  QTest::addColumn<QByteArray>( "tag" );
  QTest::addColumn<QByteArray>( "data" );
  QTest::addColumn<bool>( "complete" );
  QTest::addColumn<QList<int> >( "continuations" );

  QList<QByteArray> input;
  QList<int> continuations;
  QTest::newRow( "empty" ) << input << QByteArray() << QByteArray() << false << continuations;

  input << "*";
  QTest::newRow( "tag-only" ) << input << QByteArray("*") << QByteArray() << true << continuations;

  input.clear();
  input << "20 UID FETCH (foo)";
  QTest::newRow( "simple" ) << input << QByteArray("20")
      << QByteArray( "UID FETCH (foo)" ) << true << continuations;

  input.clear();
  input << "1 (bla (" << ") blub)";
  QTest::newRow( "parenthesis" ) << input << QByteArray("1")
      << QByteArray( "(bla () blub)" ) << true << continuations;

  input.clear();
  input << "1 {3}" << "bla";
  continuations << 0;
  QTest::newRow( "literal" ) << input << QByteArray("1") << QByteArray("{3}bla")
      << true << continuations;

  input.clear();
  input << "1 FETCH (UID 5 DATA {3}"
        << "bla"
        << " RID 5)";
  QTest::newRow( "parenthesisEnclosedLiteral" ) << input << QByteArray("1")
      << QByteArray( "FETCH (UID 5 DATA {3}bla RID 5)" ) << true << continuations;

  input.clear();
  input << "1 {3}" << "bla {4}" << "blub";
  continuations.clear();
  continuations << 0 << 1;
  QTest::newRow( "2literal" ) << input << QByteArray("1")
      << QByteArray("{3}bla {4}blub") << true << continuations;

  input.clear();
  input << "1 {4}" << "A{9}";
  continuations.clear();
  continuations << 0;
  QTest::newRow( "literal in literal" ) << input << QByteArray("1")
      << QByteArray("{4}A{9}") << true << continuations;

  input.clear();
  input << "* FETCH (UID 1 DATA {3}" << "bla" << " ENVELOPE {4}" << "blub" << " RID 5)";
  continuations.clear();
  continuations << 0 << 2;
  QTest::newRow( "enclosed2literal" ) << input << QByteArray("*")
      << QByteArray( "FETCH (UID 1 DATA {3}bla ENVELOPE {4}blub RID 5)" )
      << true << continuations;

  input.clear();
  input << "1 DATA {0}";
  continuations.clear();
  QTest::newRow( "empty literal" ) << input << QByteArray( "1" )
      << QByteArray( "DATA {0}" ) << true << continuations;
}

void ImapParserTest::testMessageParser()
{
  QFETCH( QList<QByteArray>, input );
  QFETCH( QByteArray, tag );
  QFETCH( QByteArray, data );
  QFETCH( bool, complete );
  QFETCH( QList<int>, continuations );
  QList<int> cont = continuations;

  ImapParser *parser = new ImapParser();
  QVERIFY( parser->tag().isEmpty() );
  QVERIFY( parser->data().isEmpty() );

  for ( int i = 0; i < input.count(); ++i ) {
    bool res = parser->parseNextLine( input.at( i ) );
    if ( i != input.count() - 1 )
      QVERIFY( !res );
    else
      QCOMPARE( res, complete );
    if ( parser->continuationStarted() ) {
      QVERIFY( cont.contains( i ) );
      cont.removeAll( i );
    }
  }

  QCOMPARE( parser->tag(), tag );
  QCOMPARE( parser->data(), data );
  QVERIFY( cont.isEmpty() );

  // try again, this time with a not fresh parser
  parser->reset();
  QVERIFY( parser->tag().isEmpty() );
  QVERIFY( parser->data().isEmpty() );
  cont = continuations;

  for ( int i = 0; i < input.count(); ++i ) {
    bool res = parser->parseNextLine( input.at( i ) );
    if ( i != input.count() - 1 )
      QVERIFY( !res );
    else
      QCOMPARE( res, complete );
    if ( parser->continuationStarted() ) {
      QVERIFY( cont.contains( i ) );
      cont.removeAll( i );
    }
  }

  QCOMPARE( parser->tag(), tag );
  QCOMPARE( parser->data(), data );
  QVERIFY( cont.isEmpty() );

  delete parser;
}

void ImapParserTest::testParseSequenceSet_data()
{
  QTest::addColumn<QByteArray>( "data" );
  QTest::addColumn<int>( "begin" );
  QTest::addColumn<ImapInterval::List>( "result" );
  QTest::addColumn<int>( "end" );

  QByteArray data( " 1 0:* 3:4,8:* *:5,1" );

  QTest::newRow( "empty" ) << QByteArray() << 0 << ImapInterval::List() << 0;
  QTest::newRow( "input end" ) << data << 20 << ImapInterval::List() << 20;

  ImapInterval::List result;
  result << ImapInterval( 1, 1 );
  QTest::newRow( "single value 1" ) << data << 0 << result << 2;
  QTest::newRow( "single value 2" ) << data << 1 << result << 2;
  QTest::newRow( "single value 3" ) << data << 19 << result << 20;

  result.clear();
  result << ImapInterval();
  QTest::newRow( "full interval" ) << data << 2 << result << 6;

  result.clear();
  result << ImapInterval( 3, 4 ) << ImapInterval( 8 );
  QTest::newRow( "complex 1" ) << data << 7 << result << 14;

  result.clear();
  result << ImapInterval( 0, 5 ) << ImapInterval( 1, 1 );
  QTest::newRow( "complex 2" ) << data << 14 << result << 20;
}

void ImapParserTest::testParseSequenceSet()
{
  QFETCH( QByteArray, data );
  QFETCH( int, begin );
  QFETCH( ImapInterval::List, result );
  QFETCH( int, end );

  ImapSet res;
  int pos = ImapParser::parseSequenceSet( data, res, begin );
  QCOMPARE( res.intervals(), result );
  QCOMPARE( pos, end );
}

void ImapParserTest::testParseDateTime_data()
{
  QTest::addColumn<QByteArray>( "data" );
  QTest::addColumn<int>( "begin" );
  QTest::addColumn<QDateTime>( "result" );
  QTest::addColumn<int>( "end" );

  QTest::newRow( "emtpy" ) << QByteArray() << 0 << QDateTime() << 0;

  QByteArray data( " \"28-May-2006 01:03:35 +0200\"" );
  QByteArray data2( "22-Jul-2008 16:31:48 +0000" );

  QDateTime dt( QDate( 2006, 5, 27 ), QTime( 23, 3, 35 ), Qt::UTC );
  QDateTime dt2( QDate( 2008, 7, 22 ), QTime( 16, 31, 48 ), Qt::UTC );

  QTest::newRow( "quoted 1" ) << data << 0 << dt << 29;
  QTest::newRow( "quoted 2" ) << data << 1 << dt << 29;
  QTest::newRow( "unquoted" ) << data << 2 << dt << 28;
  QTest::newRow( "unquoted2" ) << data2 << 0 << dt2 << 26;
  QTest::newRow( "invalid" ) << data << 4 << QDateTime() << 4;
}

void ImapParserTest::testParseDateTime()
{
  QFETCH( QByteArray, data );
  QFETCH( int, begin );
  QFETCH( QDateTime, result );
  QFETCH( int, end );

  QDateTime actualResult;
  int actualEnd = ImapParser::parseDateTime( data, actualResult, begin );
  QCOMPARE( actualResult, result );
  QCOMPARE( actualEnd, end );
}

void ImapParserTest::testBulkParser_data()
{
  QTest::addColumn<QByteArray>( "input" );
  QTest::addColumn<QByteArray>( "data" );

  QTest::newRow( "empty" ) << QByteArray( "* PRE {0} POST\n" ) << QByteArray("PRE {0} POST\n");
  QTest::newRow( "small block" ) << QByteArray( "* PRE {2}\nXX POST\n" )
      << QByteArray( "PRE {2}\nXX POST\n" );
  QTest::newRow( "small block 2" ) << QByteArray( "* (PRE {2}\nXX\n POST)\n" )
      << QByteArray( "(PRE {2}\nXX\n POST)\n" );
  QTest::newRow( "large block" ) << QByteArray( "* PRE {10}\n0123456789\n" )
      << QByteArray( "PRE {10}\n0123456789\n" );
  QTest::newRow( "store failure" ) << QByteArray( "3 UID STORE (FOO bar ENV {3}\n(a) HEAD {3}\na\n\n BODY {3}\nabc)\n" )
      << QByteArray( "UID STORE (FOO bar ENV {3}\n(a) HEAD {3}\na\n\n BODY {3}\nabc)\n" );
}

void ImapParserTest::testBulkParser()
{
  QFETCH( QByteArray, input );
  QFETCH( QByteArray, data );

  ImapParser *parser = new ImapParser();
  QBuffer buffer;
  buffer.setData( input );
  QVERIFY( buffer.open( QBuffer::ReadOnly ) );

  // reading continuation as a single block
  forever {
    if ( buffer.atEnd() )
      break;
    if ( parser->continuationSize() > 0 ) {
      parser->parseBlock( buffer.read( parser->continuationSize() ) );
    } else if ( buffer.canReadLine() ) {
      const QByteArray line = buffer.readLine();
      bool res = parser->parseNextLine( line );
      QCOMPARE( res, buffer.atEnd() );
    }
  }
  QCOMPARE( parser->data(), data );

  // reading continuations as smaller blocks
  buffer.reset();
  parser->reset();
  forever {
    if ( buffer.atEnd() )
      break;
    if ( parser->continuationSize() > 4 ) {
      parser->parseBlock( buffer.read( 4 ) );
    } else if ( parser->continuationSize() > 0 ) {
      parser->parseBlock( buffer.read( parser->continuationSize() ) );
    } else if ( buffer.canReadLine() ) {
      bool res = parser->parseNextLine( buffer.readLine() );
      QCOMPARE( res, buffer.atEnd() );
    }
  }

  delete parser;
}

void ImapParserTest::testJoin_data()
{
  QTest::addColumn<QList<QByteArray> >( "list" );
  QTest::addColumn<QByteArray>( "joined" );
  QTest::newRow( "empty" ) << QList<QByteArray>() << QByteArray();
  QTest::newRow( "one" ) << (QList<QByteArray>() << "abab") << QByteArray( "abab" );
  QTest::newRow( "two" ) << (QList<QByteArray>() << "abab" << "cdcd") << QByteArray( "abab cdcd" );
  QTest::newRow( "three" ) << (QList<QByteArray>() << "abab" << "cdcd" << "efef") << QByteArray( "abab cdcd efef" );
}

void ImapParserTest::testJoin()
{
  QFETCH( QList<QByteArray>, list );
  QFETCH( QByteArray, joined );
  QCOMPARE( ImapParser::join( list, " " ), joined );
}

