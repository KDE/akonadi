/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>

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

#include "imapstreamparsertest.h"

#include <QtCore/QDebug>
#include <QtCore/QVariant>
#include <QBuffer>

#define private public
#include "imapstreamparser.h"
#undef private
#define private private

Q_DECLARE_METATYPE( QList<QByteArray> )
Q_DECLARE_METATYPE( QList<int> )

using namespace Akonadi;

QString akBacktrace()
{
  QString s;

  /* FIXME: is there an equivalent for darwin, *BSD, or windows? */
#ifdef HAVE_EXECINFO_H
  void* trace[256];
  int n = backtrace(trace, 256);
  if (!n)
    return s;

  char** strings = backtrace_symbols (trace, n);

  s = QLatin1String("[\n");

  for ( int i = 0; i < n; ++i ) {
    s += QString::number(i) +
        QLatin1String(": ") +
        QLatin1String(strings[i]) + QLatin1String("\n");
  }
  s += QLatin1String("]\n");

  if (strings)
    free (strings);
#endif

  return s;
}

QTEST_MAIN( ImapStreamParserTest )


void ImapStreamParserTest::testParseQuotedString( )
{
  QByteArray result;

  QBuffer buffer;
  buffer.open( QBuffer::ReadWrite);
  ImapStreamParser parser(&buffer);
  buffer.write( QByteArray( "\"quoted \\\"NIL\\\"   string inside\"" ) );
  buffer.write( QByteArray( "quoted") );
  buffer.write( QByteArray( "\"   string inside\"" ) );
  buffer.write( QByteArray( "   string " ) );
  buffer.write("NIL \"NIL\" \"\"");
  buffer.write( " string" );
  buffer.write( "\"\\\"some \\\\ quoted stuff\\\"\"" );
  buffer.write( "LOGOUT\nFOO" );
  buffer.seek(0);

  try {
  // the whole thing
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray( "quoted \"NIL\"   string inside" ) );

  // unqoted string
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray( "quoted" ) );

  // whitespaces in qouted string
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray( "   string inside" ) );

  // whitespaces before unquoted string
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray( "string" ) );


  // NIL and emptyness tests

  // unqoted NIL
  result = parser.parseQuotedString();
  QVERIFY( result.isNull() );

  // quoted NIL
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray( "NIL" ) );

  // quoted empty string
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray( "" ) );

  // unquoted string at input end
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray( "string" ) );

//   // out of bounds access
//   result = parser.parseQuotedString();
//   QVERIFY( result.isEmpty() );

  // de-quoting
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray( "\"some \\ quoted stuff\"" ) );

  // linebreak as separator
  result = parser.parseQuotedString();
  QCOMPARE( result, QByteArray("LOGOUT") );
  } catch ( const Akonadi::Exception &e ) {
    qDebug() << "Caught exception: " << e.type() << " : " <<
        e.what();
  } catch (...)
  {
    qDebug() << "Unknown exception caught: " << akBacktrace();
  }
}


void ImapStreamParserTest::testParseString( )
{

  QBuffer buffer;
  buffer.open( QBuffer::ReadWrite);
  ImapStreamParser parser(&buffer);

  buffer.write( QByteArray( "\"quoted\" unquoted {7}\nliteral {0}\n " ) );
  buffer.seek(0);

  QByteArray result;

  try {
  // quoted strings
  result = parser.readString();
  QCOMPARE( result, QByteArray( "quoted" ) );

  // unquoted string
  result = parser.readString( );
  QCOMPARE( result, QByteArray( "unquoted" ) );

  // literal string
  result = parser.readString( );
  QCOMPARE( result, QByteArray( "literal" ) );

  // empty literal string
  result = parser.readString();
  QCOMPARE( result, QByteArray( "" ) );

  // out of bounds access

  //this results in an ImapStreamParser exception
  result = parser.readString();
  QCOMPARE( result, QByteArray() );
  } catch ( const Akonadi::Exception &e ) {
    qDebug() << "Caught exception: " << e.type() << " : " <<
        e.what();
  } catch (...)
  {
    qDebug() << "Unknown exception caught: " << akBacktrace();
  }
}

void ImapStreamParserTest::testParseParenthesizedList( )
{
  QBuffer buffer;
  buffer.open( QBuffer::ReadWrite);
  ImapStreamParser parser(&buffer);

  buffer.write( QByteArray( "() ( )" ) );
  buffer.write( QByteArray( "(entry1 \"entry2()\" (sub list) \")))\" {6}\nentry3) " ) );
  buffer.write( QByteArray( "some_list-less_text" ) );
  buffer.write( QByteArray( "(foo {6}\n\n\nbar\n bla)" ));
  buffer.seek(0);

  QList<QByteArray>result;

  try {
  // empty lists
  result = parser.readParenthesizedList();
  QVERIFY( result.isEmpty() );

  result = parser.readParenthesizedList();
  QVERIFY( result.isEmpty() );

  // complex list with all kind of entries
  qDebug() << "complex test";
  result = parser.readParenthesizedList();
  QList<QByteArray> reference;
  reference << "entry1";
  reference << "entry2()";
  reference << "(sub list)";
  reference << ")))";
  reference << "entry3";


  QCOMPARE( result, reference );

  // no list at all
  result = parser.readParenthesizedList(  );
  QVERIFY( result.isEmpty() );

  parser.readString(); //just to advance in the data

//   // out of bounds access
//   result = parser.readParenthesizedList( input, result, input.length() );
//   QVERIFY( result.isEmpty() );
//   QCOMPARE( consumed, input.length() );

  // newline literal (based on itemappendtest bug)
  result = parser.readParenthesizedList(  );
  reference.clear();
  reference << "foo";
  reference << "\n\nbar\n";
  reference << "bla";

  QCOMPARE( result, reference );
  } catch ( const Akonadi::Exception &e ) {
    qDebug() << "Caught exception: " << e.type() << " : " <<
        e.what();
  } catch (...)
  {
    qDebug() << "Unknown exception caught: " << akBacktrace();
  }

}


void ImapStreamParserTest::testParseNumber()
{
  QBuffer buffer;
  buffer.open( QBuffer::ReadWrite | QIODevice::Truncate );
  ImapStreamParser parser(&buffer);

  buffer.write( QByteArray( " " ) );
  buffer.seek(0);

  qint64 result;
  bool ok;

  try {
  // empty string
  result = parser.readNumber( &ok );
  QCOMPARE( ok, false );

  buffer.close();
  buffer.open( QBuffer::ReadWrite | QIODevice::Truncate );
  buffer.write( QByteArray( " 1 23 4" ) );
  buffer.seek(0);
  // leading spaces at the beginning
  result = parser.readNumber( &ok );
  QCOMPARE( ok, true );
  QCOMPARE( result, 1ll );

  // multiple digits
  result = parser.readNumber(  &ok );
  QCOMPARE( ok, true );
  QCOMPARE( result, 23ll );

  // number at input end
  result = parser.readNumber(&ok );
  QCOMPARE( ok, true );
  QCOMPARE( result, 4ll );

  // out of bounds access - throws an exception
  result = parser.readNumber(  &ok );
  QCOMPARE( ok, false );
} catch ( const Akonadi::Exception &e ) {
  qDebug() << "Caught exception: " << e.type() << " : " <<
      e.what();
} catch (...)
{
  qDebug() << "Unknown exception caught: " << akBacktrace();
}
}


void ImapStreamParserTest::testParseSequenceSet_data()
{
  QTest::addColumn<QByteArray>( "data" );
  QTest::addColumn<ImapInterval::List>( "result" );

  QByteArray data( " 1 0:* 3:4,8:* *:5,1" );

  QTest::newRow( "empty" ) << QByteArray() << ImapInterval::List() ;

  ImapInterval::List result;
  result << ImapInterval( 1, 1 );
  data = " 1 ";
  QTest::newRow( "single value" ) << data  << result;

  result.clear();
  result << ImapInterval();
  data = "  0:* ";
  QTest::newRow( "full interval" ) << data << result;

  data = " 3:4,8:*";
  result.clear();
  result << ImapInterval( 3, 4 ) << ImapInterval( 8 );
  QTest::newRow( "complex 1" ) << data << result ;

  data = " *:5,1";
  result.clear();
  result << ImapInterval( 0, 5 ) << ImapInterval( 1, 1 );
  QTest::newRow( "complex 2" ) << data  << result ;
}

void ImapStreamParserTest::testParseSequenceSet()
{
  QFETCH( QByteArray, data );
  QFETCH( ImapInterval::List, result );

  QBuffer buffer;
  buffer.open( QBuffer::ReadWrite);
  ImapStreamParser parser(&buffer);
  buffer.write( data );
  buffer.seek(0);

  ImapSet res;
  try {
    res = parser.readSequenceSet();
    QCOMPARE( res.intervals(), result );
} catch ( const Akonadi::Exception &e ) {
  qDebug() << "Caught exception: " << e.type() << " : " <<
      e.what();
} catch (...)
{
  qDebug() << "Unknown exception caught: " << akBacktrace();
}
}


void ImapStreamParserTest::testParseDateTime_data()
{
  QTest::addColumn<QByteArray>( "data" );
  QTest::addColumn<QDateTime>( "result" );

  QTest::newRow( "empty" ) << QByteArray() <<  QDateTime() ;

  QByteArray data( " \"28-May-2006 01:03:35 +0200\"" );
  QByteArray data2( "22-Jul-2008 16:31:48 +0000" );
  QByteArray data3( "ul-2008 16:31:48 +0000" );

  QDateTime dt( QDate( 2006, 5, 27 ), QTime( 23, 3, 35 ), Qt::UTC );
  QDateTime dt2( QDate( 2008, 7, 22 ), QTime( 16, 31, 48 ), Qt::UTC );

  QTest::newRow( "quoted" ) << data  << dt ;
  QTest::newRow( "unquoted" ) << data2 <<  dt2 ;
  QTest::newRow( "invalid" ) << data3 << QDateTime();
}

void ImapStreamParserTest::testParseDateTime()
{
  QFETCH( QByteArray, data );
  QFETCH( QDateTime, result );

  QBuffer buffer;
  buffer.open( QBuffer::ReadWrite);
  ImapStreamParser parser(&buffer);
  buffer.write( data );
  buffer.seek(0);

  QDateTime actualResult;
  try {
    actualResult = parser.readDateTime();
    QCOMPARE( actualResult, result );
  } catch ( const Akonadi::Exception &e ) {
    qDebug() << "Caught exception: " << e.type() << " : " <<
        e.what();
  } catch (...)
  {
    qDebug() << "Unknown exception caught: " << akBacktrace();
  }
}

void ImapStreamParserTest::testReadUntilCommandEnd()
{
  QByteArray input( "2 UID STORE 2 NOREV SIZE 0 (+FLAGS.SILENT (\\Deleted))\n3 EXPUNGE\n" );
  QBuffer buffer( &input, this );
  buffer.open( QIODevice::ReadOnly );
  ImapStreamParser parser( &buffer );

  try {
    QCOMPARE( parser.readString(), QByteArray( "2" ) );
    QCOMPARE( parser.readString(), QByteArray( "UID" ) );
    QCOMPARE( parser.readString(), QByteArray( "STORE" ) );
    QCOMPARE( parser.readString(), QByteArray( "2" ) );
    QCOMPARE( parser.readString(), QByteArray( "NOREV" ) );
    QCOMPARE( parser.readString(), QByteArray( "SIZE" ) );
    QCOMPARE( parser.readString(), QByteArray( "0" ) );
    parser.stripLeadingSpaces();
    QCOMPARE( static_cast<char>( parser.readChar() ), '(' );
    QVERIFY( !parser.atCommandEnd() );
    parser.readUntilCommandEnd();
    QCOMPARE( parser.readString(), QByteArray( "3" ) );
  } catch ( const Akonadi::Exception &e ) {
    qDebug() << e.type() << e.what();
    QFAIL( "Exception caught" );
  }
}

// real bug, infinite loop with { as first char in a quoted string
void ImapStreamParserTest::testReadUntilCommandEnd2()
{
  QByteArray input( "4 MODIFY 595 MIMETYPE (message/rfc822 inode/directory) NAME \"child2\" REMOTEID \"{b42}\"\nNEXTCOMMAND\n" );
  QBuffer buffer( &input, this );
  buffer.open( QIODevice::ReadOnly );
  ImapStreamParser parser( &buffer );

  try {
    QCOMPARE( parser.readString(), QByteArray( "4" ) );
    QCOMPARE( parser.readString(), QByteArray( "MODIFY" ) );
    QCOMPARE( parser.readString(), QByteArray( "595" ) );
    QCOMPARE( parser.readUntilCommandEnd(), QByteArray( " MIMETYPE (message/rfc822 inode/directory) NAME \"child2\" REMOTEID \"{b42}\"\n" ) );
    QCOMPARE( parser.readString(), QByteArray( "NEXTCOMMAND" ) );
  } catch ( const Akonadi::Exception &e ) {
    qDebug() << e.type() << e.what();
    QFAIL( "Exception caught" );
  }
}


#include "imapstreamparsertest.moc"
