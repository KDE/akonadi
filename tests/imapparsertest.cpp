/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "imapparser.h"
#include "imapparsertest.h"
#include <qtest_kde.h>

#include <QDebug>

using namespace PIM;

QTEST_KDEMAIN( ImapParserTest, NoGUI );

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
}

void ImapParserTest::testParseParenthesizedList( )
{
  QByteArray input;
  QList<QByteArray> result;
  int consumed;

  // empty lists
  input = "() ( )";
  consumed = ImapParser::parseParenthesizedList( input, result, 0 );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, 2 );

  consumed = ImapParser::parseParenthesizedList( input, result, 2 );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, 6 );

  // complex list with all kind of entries
  input = "(entry1 \"entry2()\" (sub list) \")))\" {6}\nentry3) end";
  consumed = ImapParser::parseParenthesizedList( input, result, 0 );
  QList<QByteArray> reference;
  reference << "entry1";
  reference << "entry2()";
  reference << "(sub list)";
  reference << ")))";
  reference << "entry3";
  QCOMPARE( result, reference );
  QCOMPARE( consumed, 47 );

  // no list at all
  input = "some list-less text";
  consumed = ImapParser::parseParenthesizedList( input, result, 0 );
  QVERIFY( result.isEmpty() );
  QCOMPARE( consumed, 0 );
}



#include "imapparsertest.moc"
