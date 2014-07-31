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
#include <QtTest/QTest>

#include "handlerhelper.cpp"

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Akonadi::Server::Collection)
Q_DECLARE_METATYPE(QStack<Akonadi::Server::Collection>)

class HandlerHelperTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCachePolicyToByteArray_data()
    {
        QTest::addColumn<Collection>("collection");
        QTest::addColumn<QByteArray>("result");

        Collection c;
        c.setCachePolicyInherit(true);
        QTest::newRow("inherit") << c << QByteArray("CACHEPOLICY (INHERIT true INTERVAL -1 CACHETIMEOUT -1 SYNCONDEMAND false LOCALPARTS ())");

        c.setCachePolicyInherit(false);
        c.setCachePolicyCheckInterval(1);
        c.setCachePolicyCacheTimeout(2);
        c.setCachePolicyLocalParts(QLatin1String("PART1 PART2"));
        c.setCachePolicySyncOnDemand(true);
        QTest::newRow("non-inherit") << c << QByteArray("CACHEPOLICY (INHERIT false INTERVAL 1 CACHETIMEOUT 2 SYNCONDEMAND true LOCALPARTS (PART1 PART2))");
    }

    void testCachePolicyToByteArray()
    {
        QFETCH(Collection, collection);
        QFETCH(QByteArray, result);

        const QByteArray realResult = HandlerHelper::cachePolicyToByteArray(collection);
        QCOMPARE(realResult, result);
    }

    void testAncestorsToByteArray_data()
    {
        QTest::addColumn<int>("depth");
        QTest::addColumn<QStack<Collection> >("ancestors");
        QTest::addColumn<QByteArray>("result");

        QTest::newRow("empty0") << 0 << QStack<Collection>() << QByteArray();
        QTest::newRow("empty1") << 1 << QStack<Collection>() << QByteArray("ANCESTORS ((0 \"\"))");
        QTest::newRow("empty2") << 2 << QStack<Collection>() << QByteArray("ANCESTORS ((0 \"\"))");

        QStack<Collection> ancestors;
        Collection c;

        c.setId(10);
        c.setRemoteId(QLatin1String("sl\\ash"));
        ancestors.push(c);
        QTest::newRow("slash") << 1 << ancestors << QByteArray("ANCESTORS ((10 \"sl\\\\ash\"))");

        ancestors.clear();
        c.setId(1);
        c.setRemoteId(QLatin1String("r1"));
        ancestors.push(c);
        QTest::newRow("one1") << 1 << ancestors << QByteArray("ANCESTORS ((1 \"r1\"))");
        QTest::newRow("one2") << 2 << ancestors << QByteArray("ANCESTORS ((1 \"r1\") (0 \"\"))");
        QTest::newRow("one3") << 3 << ancestors << QByteArray("ANCESTORS ((1 \"r1\") (0 \"\"))");

        c.setId(2);
        c.setRemoteId(QLatin1String("r2"));
        ancestors.push(c);
        QTest::newRow("two1") << 1 << ancestors << QByteArray("ANCESTORS ((2 \"r2\"))");
        QTest::newRow("two2") << 2 << ancestors << QByteArray("ANCESTORS ((2 \"r2\") (1 \"r1\"))");
        QTest::newRow("two3") << 3 << ancestors << QByteArray("ANCESTORS ((2 \"r2\") (1 \"r1\") (0 \"\"))");
        QTest::newRow("two4") << 4 << ancestors << QByteArray("ANCESTORS ((2 \"r2\") (1 \"r1\") (0 \"\"))");
    }

    void testAncestorsToByteArray()
    {
        QFETCH(int, depth);
        QFETCH(QStack<Collection>, ancestors);
        QFETCH(QByteArray, result);

        const QByteArray realResult = HandlerHelper::ancestorsToByteArray(depth, ancestors);
        QCOMPARE(realResult, result);
    }

    void testParseDepth_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<int>("output");
        QTest::addColumn<bool>("success");

        QTest::newRow("0") << QByteArray("0") << 0 << true;
        QTest::newRow("1") << QByteArray("1") << 1 << true;
        QTest::newRow("INF") << QByteArray("INF") << INT_MAX << true;

        QTest::newRow("empty") << QByteArray() << 0 << false;
        QTest::newRow("invalid") << QByteArray("foo") << 0 << false;
    }

    void testParseDepth()
    {
        QFETCH(QByteArray, input);
        QFETCH(int, output);
        QFETCH(bool, success);

        bool didThrow = false;
        try {
            const int result = HandlerHelper::parseDepth(input);
            QCOMPARE(result, output);
        } catch (const ImapParserException &e) {
            if (success) {
                qDebug() << e.what();
            }
            didThrow = true;
        }
        QCOMPARE(didThrow, !success);
    }
};

QTEST_MAIN(HandlerHelperTest)

#include "handlerhelpertest.moc"
