/*
    Copyright (c) 2013 Volker Krause <vkrause@kde.org>
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "handler/fetchscope.cpp"
#include "imapstreamparser.h"

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QVector<QByteArray>)

class FetchScopeTest : public QObject
{
    Q_OBJECT
public:
    FetchScopeTest()
        : QObject()
    {
        qRegisterMetaType<QVector<QByteArray> >();
    }

private Q_SLOTS:
    void testCommandParsing_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QVector<QByteArray> >("requestedParts");
        QTest::addColumn<QStringList>("requestedPayloads");
        QTest::addColumn<QDateTime>("changedSince");
        QTest::addColumn<int>("ancestorDepth");
        QTest::addColumn<bool>("cacheOnly");
        QTest::addColumn<bool>("checkCachedPayloadPartsOnly");
        QTest::addColumn<bool>("fullPayload");
        QTest::addColumn<bool>("allAttrs");
        QTest::addColumn<bool>("sizeRequested");
        QTest::addColumn<bool>("mTimeRequested");
        QTest::addColumn<bool>("externalPayloadSupported");
        QTest::addColumn<bool>("remoteRevisionRequested");
        QTest::addColumn<bool>("ignoreErrors");
        QTest::addColumn<bool>("flagsRequested");
        QTest::addColumn<bool>("remoteIdRequested");
        QTest::addColumn<bool>("gidRequested");

        QTest::newRow("Normal fetch scope")
                << "CACHEONLY EXTERNALPAYLOAD IGNOREERRORS CHANGEDSINCE 1374150376 ANCESTORS 42 (DATETIME REMOTEREVISION REMOTEID GID FLAGS SIZE PLD:RFC822 ATR::MyAttr)\n"
                << (QVector<QByteArray>() << "PLD:RFC822" << "ATR:MyAttr")
                << (QStringList() << QLatin1String("PLD:RFC822"))
                << QDateTime::fromTime_t(1374150376)
                << 42
                << true << false << false << false << true << true << true << true
                << true << true << true << true;

        QTest::newRow("fullpayload")
                << "FULLPAYLOAD ()"
                << (QVector<QByteArray>() << "PLD:RFC822")
                << QStringList()
                << QDateTime()
                << 0
                << false << false << true << false << false << false << false << false
                << false << false << false << false;
    }

    void testCommandParsing()
    {
        QFETCH(QString, input);
        QFETCH(QVector<QByteArray>, requestedParts);
        QFETCH(QStringList, requestedPayloads);
        QFETCH(QDateTime, changedSince);
        QFETCH(int, ancestorDepth);
        QFETCH(bool, cacheOnly);
        QFETCH(bool, checkCachedPayloadPartsOnly);
        QFETCH(bool, fullPayload);
        QFETCH(bool, allAttrs);
        QFETCH(bool, sizeRequested);
        QFETCH(bool, mTimeRequested);
        QFETCH(bool, externalPayloadSupported);
        QFETCH(bool, remoteRevisionRequested);
        QFETCH(bool, ignoreErrors);
        QFETCH(bool, flagsRequested);
        QFETCH(bool, remoteIdRequested);
        QFETCH(bool, gidRequested);

        FetchScope fs;
        QVERIFY(!fs.remoteRevisionRequested());

        QByteArray ba(input.toLatin1());
        QBuffer buffer(&ba, this);
        buffer.open(QIODevice::ReadOnly);
        ImapStreamParser parser(&buffer);

        fs = FetchScope(&parser);
        QCOMPARE(fs.requestedParts().size(), requestedParts.size());
        QCOMPARE(fs.requestedPayloads().size(), requestedPayloads.size());
        QCOMPARE(fs.changedSince(), changedSince);
        QCOMPARE(fs.ancestorDepth(), ancestorDepth);
        QCOMPARE(fs.cacheOnly(), cacheOnly);
        QCOMPARE(fs.checkCachedPayloadPartsOnly(), checkCachedPayloadPartsOnly);
        QCOMPARE(fs.fullPayload(), fullPayload);
        QCOMPARE(fs.allAttributes(), allAttrs);
        QCOMPARE(fs.sizeRequested(), sizeRequested);
        QCOMPARE(fs.mTimeRequested(), mTimeRequested);
        QCOMPARE(fs.externalPayloadSupported(), externalPayloadSupported);
        QCOMPARE(fs.remoteRevisionRequested(), remoteRevisionRequested);
        QCOMPARE(fs.ignoreErrors(), ignoreErrors);
        QCOMPARE(fs.flagsRequested(), flagsRequested);
        QCOMPARE(fs.remoteIdRequested(), remoteIdRequested);
        QCOMPARE(fs.gidRequested(), gidRequested);
    }
};

QTEST_MAIN(FetchScopeTest)

#include "fetchscopetest.moc"
