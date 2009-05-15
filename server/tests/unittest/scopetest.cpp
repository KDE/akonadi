/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include <QObject>
#include <QtTest>

#include "handler/scope.cpp"
#include "imapstreamparser.h"

using namespace Akonadi;

class ScopeTest : public QObject
{
  Q_OBJECT
  private slots:
    void testUidSet()
    {
      Scope scope( Scope::Uid );

      QByteArray input( "1:3,6 FOO\n" );
      QBuffer buffer( &input, this );
      buffer.open( QIODevice::ReadOnly );
      ImapStreamParser parser( &buffer );

      scope.parseScope( &parser );
      QCOMPARE( parser.readRemainingData(), QByteArray( " FOO\n" ) );

      QCOMPARE( scope.scope(), Scope::Uid );
      QCOMPARE( scope.uidSet().toImapSequenceSet(), QByteArray( "1:3,6" ) );
      QVERIFY( scope.ridSet().isEmpty() );
    }

    void testRid_data()
    {
      QTest::addColumn<QByteArray>( "input" );
      QTest::addColumn<QString>( "rid" );
      QTest::addColumn<QByteArray>( "remainder" );

      QTest::newRow( "no list, remainder" )
        << QByteArray( "\"(my remote id)\" FOO\n" )
        << QString::fromLatin1( "(my remote id)" )
        << QByteArray( " FOO\n" );
      QTest::newRow( "list, no remainder" ) << QByteArray( "(\"A\")" ) << QString::fromLatin1( "A" ) << QByteArray();
      QTest::newRow( "list, no reaminder, leading space" )
        << QByteArray( " (\"A\")\n" ) << QString::fromLatin1( "A" ) << QByteArray( "\n" );
    }

    void testRid()
    {
      QFETCH( QByteArray, input );
      QFETCH( QString, rid );
      QFETCH( QByteArray, remainder );

      Scope scope( Scope::Rid );

      QBuffer buffer( &input, this );
      buffer.open( QIODevice::ReadOnly );
      ImapStreamParser parser( &buffer );

      scope.parseScope( &parser );
      QCOMPARE( parser.readRemainingData(), remainder );

      QCOMPARE( scope.scope(), Scope::Rid );
      QVERIFY( scope.uidSet().isEmpty() );
      QCOMPARE( scope.ridSet().size(), 1 );
      QCOMPARE( scope.ridSet().first(), rid );
    }

    void testRidSet()
    {
      Scope scope( Scope::Rid );

      QByteArray input( "(\"my first remote id\" \"my second remote id\") FOO\n" );
      QBuffer buffer( &input, this );
      buffer.open( QIODevice::ReadOnly );
      ImapStreamParser parser( &buffer );

      scope.parseScope( &parser );
      QCOMPARE( parser.readRemainingData(), QByteArray( " FOO\n" ) );

      QCOMPARE( scope.scope(), Scope::Rid );
      QVERIFY( scope.uidSet().isEmpty() );
      QCOMPARE( scope.ridSet().size(), 2 );
      QCOMPARE( scope.ridSet().first(), QLatin1String( "my first remote id" ) );
      QCOMPARE( scope.ridSet().last(), QLatin1String( "my second remote id" ) );
    }
};

QTEST_MAIN( ScopeTest )

#include "scopetest.moc"
