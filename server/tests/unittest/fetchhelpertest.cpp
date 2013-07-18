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
      FetchHelper fh(0, Scope(Scope::Invalid));
      QVERIFY(!fh.mRemoteRevisionRequested);

      QByteArray input( "CACHEONLY EXTERNALPAYLOAD IGNOREERRORS CHANGEDSINCE 1374150376 ANCESTORS 42 (DATETIME REMOTEREVISION REMOTEID FLAGS SIZE PLD:RFC822 ATR::MyAttr)\n" );
      QBuffer buffer( &input, this );
      buffer.open( QIODevice::ReadOnly );
      ImapStreamParser parser( &buffer );
      fh.setStreamParser( &parser );

      fh.parseCommandStream();

      QVERIFY(fh.mRemoteRevisionRequested);
      QVERIFY(fh.mSizeRequested);
      QVERIFY(fh.mCacheOnly);
      QVERIFY(fh.mExternalPayloadSupported);
      QCOMPARE(fh.mAncestorDepth, 42);
      QCOMPARE(fh.mChangedSince.toTime_t(), 1374150376u);
      QVERIFY(fh.mIgnoreErrors);
      QVERIFY(!fh.mFullPayload);
      QCOMPARE(fh.mRequestedParts.size(), 2);
      QVERIFY(!fh.mAllAttrs);
      QVERIFY(fh.mMTimeRequested);
      QVERIFY(fh.mRemoteIdRequested);
    }
};

QTEST_MAIN( FetchHelperTest )

#include "fetchhelpertest.moc"
