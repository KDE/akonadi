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
#include <QtCore/QBuffer>

#include "handler/scope.cpp"
#include "imapstreamparser.h"

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Scope::SelectionScope)

class ScopeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testUidSet()
    {
        Scope scope(Scope::Uid);

        QByteArray input("1:3,6 FOO\n");
        QBuffer buffer(&input, this);
        buffer.open(QIODevice::ReadOnly);
        ImapStreamParser parser(&buffer);

        scope.parseScope(&parser);
        QCOMPARE(parser.readRemainingData(), QByteArray(" FOO\n"));

        QCOMPARE(scope.scope(), Scope::Uid);
        QCOMPARE(scope.uidSet().toImapSequenceSet(), QByteArray("1:3,6"));
        QVERIFY(scope.ridSet().isEmpty());
    }

    void testRid_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QString>("rid");
        QTest::addColumn<QByteArray>("remainder");

        QTest::newRow("no list, remainder")
                << QByteArray("\"(my remote id)\" FOO\n")
                << QString::fromLatin1("(my remote id)")
                << QByteArray(" FOO\n");
        QTest::newRow("list, no remainder") << QByteArray("(\"A\")") << QString::fromLatin1("A") << QByteArray();
        QTest::newRow("list, no reaminder, leading space")
                << QByteArray(" (\"A\")\n") << QString::fromLatin1("A") << QByteArray("\n");
    }

    void testRid()
    {
        QFETCH(QByteArray, input);
        QFETCH(QString, rid);
        QFETCH(QByteArray, remainder);

        Scope scope(Scope::Rid);

        QBuffer buffer(&input, this);
        buffer.open(QIODevice::ReadOnly);
        ImapStreamParser parser(&buffer);

        scope.parseScope(&parser);
        QCOMPARE(parser.readRemainingData(), remainder);

        QCOMPARE(scope.scope(), Scope::Rid);
        QVERIFY(scope.uidSet().isEmpty());
        QCOMPARE(scope.ridSet().size(), 1);
        QCOMPARE(scope.ridSet().first(), rid);
    }

    void testRidSet()
    {
        Scope scope(Scope::Rid);

        QByteArray input("(\"my first remote id\" \"my second remote id\") FOO\n");
        QBuffer buffer(&input, this);
        buffer.open(QIODevice::ReadOnly);
        ImapStreamParser parser(&buffer);

        scope.parseScope(&parser);
        QCOMPARE(parser.readRemainingData(), QByteArray(" FOO\n"));

        QCOMPARE(scope.scope(), Scope::Rid);
        QVERIFY(scope.uidSet().isEmpty());
        QCOMPARE(scope.ridSet().size(), 2);
        QCOMPARE(scope.ridSet().first(), QLatin1String("my first remote id"));
        QCOMPARE(scope.ridSet().last(), QLatin1String("my second remote id"));
    }

    void testHRID_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QStringList>("result");
        QTest::addColumn<bool>("success");

        QTest::newRow("null") << QByteArray() << QStringList() << false;
        QTest::newRow("empty") << QByteArray("()") << QStringList() << true;
        QTest::newRow("root") << QByteArray("((0 \"\"))") << (QStringList() << QLatin1String("")) << true;
        QTest::newRow("chain") << QByteArray("((1 \"r1\") (2 \"r2\"))") << (QStringList() << QLatin1String("r1") << QLatin1String("r2")) << true;
        QTest::newRow("invalid 1") << QByteArray("(foo)") << QStringList() << false;
        QTest::newRow("invalid 2") << QByteArray("foo") << QStringList() << false;
        QTest::newRow("invalid 3") << QByteArray("((foo)") << QStringList() << false;
        QTest::newRow("invalid 4") << QByteArray("(()foo)") << QStringList() << false;
    }

    void testHRID()
    {
        QFETCH(QByteArray, input);
        QFETCH(QStringList, result);
        QFETCH(bool, success);

        QBuffer buffer(&input, this);
        buffer.open(QIODevice::ReadOnly);
        ImapStreamParser parser(&buffer);

        Scope scope(Scope::HierarchicalRid);
        bool didThrow = false;
        try {
            scope.parseScope(&parser);
        } catch (const Akonadi::Server::Exception &e) {
            didThrow = true;
            if (success) {
                qDebug() << e.type() << e.what();
            }
        }
        QCOMPARE(didThrow, !success);
        if (success) {
            QCOMPARE(scope.ridChain(), result);
        }
    }

    void testSelectionScopeParsing_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<Scope::SelectionScope>("scope");

        QTest::newRow("empty") << QByteArray() << Scope::None;
        QTest::newRow("UID") << QByteArray("UID") << Scope::Uid;
        QTest::newRow("RID") << QByteArray("RID") << Scope::Rid;
        QTest::newRow("HRID") << QByteArray("HRID") << Scope::HierarchicalRid;
        QTest::newRow("GID") << QByteArray("GID") << Scope::Gid;
        QTest::newRow("none") << QByteArray("1:*") << Scope::None;
    }

    void testSelectionScopeParsing()
    {
        QFETCH(QByteArray, input);
        QFETCH(Scope::SelectionScope, scope);

        QCOMPARE(Scope::selectionScopeFromByteArray(input), scope);
    }

    void testGid_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QString>("gid");
        QTest::addColumn<QByteArray>("remainder");

        QTest::newRow("no list, remainder")
                << QByteArray("\"(my remote id)\" FOO\n")
                << QString::fromLatin1("(my remote id)")
                << QByteArray(" FOO\n");
        QTest::newRow("list, no remainder") << QByteArray("(\"A\")") << QString::fromLatin1("A") << QByteArray();
        QTest::newRow("list, no reaminder, leading space")
                << QByteArray(" (\"A\")\n") << QString::fromLatin1("A") << QByteArray("\n");
    }

    void testGid()
    {
        QFETCH(QByteArray, input);
        QFETCH(QString, gid);
        QFETCH(QByteArray, remainder);

        Scope scope(Scope::Gid);

        QBuffer buffer(&input, this);
        buffer.open(QIODevice::ReadOnly);
        ImapStreamParser parser(&buffer);

        scope.parseScope(&parser);
        QCOMPARE(parser.readRemainingData(), remainder);

        QCOMPARE(scope.scope(), Scope::Gid);
        QVERIFY(scope.uidSet().isEmpty());
        QCOMPARE(scope.gidSet().size(), 1);
        QCOMPARE(scope.gidSet().first(), gid);
    }

    void testGidSet()
    {
        Scope scope(Scope::Gid);

        QByteArray input("(\"my first remote id\" \"my second remote id\") FOO\n");
        QBuffer buffer(&input, this);
        buffer.open(QIODevice::ReadOnly);
        ImapStreamParser parser(&buffer);

        scope.parseScope(&parser);
        QCOMPARE(parser.readRemainingData(), QByteArray(" FOO\n"));

        QCOMPARE(scope.scope(), Scope::Gid);
        QVERIFY(scope.uidSet().isEmpty());
        QCOMPARE(scope.gidSet().size(), 2);
        QCOMPARE(scope.gidSet().first(), QLatin1String("my first remote id"));
        QCOMPARE(scope.gidSet().last(), QLatin1String("my second remote id"));
    }
};

QTEST_MAIN(ScopeTest)

#include "scopetest.moc"
