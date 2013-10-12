/*
    Copyright (c) 2013 Volker Krause <vkrause@kde.org>

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
#include <QtTest/QTest>
#include <QtCore/QBuffer>

#include "handler/fetchhelper.cpp"
#include "imapstreamparser.h"

using namespace Akonadi;

class FetchHelperTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:
    void testCommandParsing()
    {
      FetchHelper fh( 0, Scope( Scope::Invalid ) );
      QVERIFY( !fh.mFetchScope.remoteRevisionRequested() );

      QByteArray input( "CACHEONLY EXTERNALPAYLOAD IGNOREERRORS CHANGEDSINCE 1374150376 ANCESTORS 42 (DATETIME REMOTEREVISION REMOTEID GID FLAGS SIZE PLD:RFC822 ATR::MyAttr)\n" );
      QBuffer buffer( &input, this );
      buffer.open( QIODevice::ReadOnly );
      ImapStreamParser parser( &buffer );
      fh.setStreamParser( &parser );

      fh.mFetchScope.d->parseCommandStream();

      QVERIFY( fh.mFetchScope.remoteRevisionRequested() );
      QVERIFY( fh.mFetchScope.sizeRequested() );
      QVERIFY( fh.mFetchScope.cacheOnly() );
      QVERIFY( fh.mFetchScope.externalPayloadSupported() );
      QCOMPARE( fh.mFetchScope.ancestorDepth(), 42 );
      QCOMPARE( fh.mFetchScope.changedSince().toTime_t(), 1374150376u );
      QVERIFY( fh.mFetchScope.ignoreErrors() );
      QVERIFY( !fh.mFetchScope.fullPayload() );
      QCOMPARE( fh.mFetchScope.requestedParts().size(), 2 );
      QVERIFY( !fh.mFetchScope.allAttrs() );
      QVERIFY( fh.mFetchScope.mTimeRequested() );
      QVERIFY( fh.mFetchScope.remoteIdRequested() );
      QVERIFY( fh.mFetchScope.gidRequested() );

      // full payload special case
      input = "FULLPAYLOAD ()";
      buffer.setBuffer( &input );
      buffer.open( QIODevice::ReadOnly );
      parser = ImapStreamParser( &buffer );

      FetchHelper fh2( 0, Scope( Scope::Invalid ) );
      fh2.setStreamParser( &parser );
      fh2.mFetchScope.d->parseCommandStream();

      QVERIFY( !fh2.mFetchScope.remoteRevisionRequested() );
      QVERIFY( !fh2.mFetchScope.sizeRequested() );
      QVERIFY( !fh2.mFetchScope.cacheOnly() );
      QVERIFY( !fh2.mFetchScope.externalPayloadSupported() );
      QCOMPARE( fh2.mFetchScope.ancestorDepth(), 0 );
      QVERIFY( fh2.mFetchScope.changedSince().isNull() );
      QVERIFY( !fh2.mFetchScope.ignoreErrors() );
      QVERIFY( fh2.mFetchScope.fullPayload() );
      QCOMPARE( fh2.mFetchScope.requestedParts().size(), 1 );
      QCOMPARE( fh2.mFetchScope.requestedParts().at( 0 ), QByteArray( "PLD:RFC822" ) );
      QVERIFY( !fh2.mFetchScope.allAttrs() );
      QVERIFY( !fh2.mFetchScope.mTimeRequested() );
      QVERIFY( !fh2.mFetchScope.remoteIdRequested() );
      QVERIFY( !fh2.mFetchScope.gidRequested() );
    }
};

QTEST_MAIN( FetchHelperTest )

#include "fetchhelpertest.moc"
