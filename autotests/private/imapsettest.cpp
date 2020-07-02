/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imapsettest.h"
#include "private/imapset_p.h"

#include <QTest>

QTEST_MAIN(ImapSetTest)

Q_DECLARE_METATYPE(QList<qint64>)

using namespace Akonadi;

void ImapSetTest::testAddList_data()
{
    QTest::addColumn<QList<qint64> >("source");
    QTest::addColumn<ImapInterval::List>("intervals");
    QTest::addColumn<QByteArray>("seqset");

    // empty set
    QList<qint64> source;
    ImapInterval::List intervals;
    QTest::newRow("empty") << source << intervals << QByteArray();

    // single value
    source << 4;
    intervals << ImapInterval(4, 4);
    QTest::newRow("single value") << source << intervals << QByteArray("4");

    // single 2-value interval
    source << 5;
    intervals.clear();
    intervals << ImapInterval(4, 5);
    QTest::newRow("single 2 interval") << source << intervals << QByteArray("4:5");

    // single large interval
    source << 6 << 3 << 7 << 2  << 8;
    intervals.clear();
    intervals << ImapInterval(2, 8);
    QTest::newRow("single 7 interval") << source << intervals << QByteArray("2:8");

    // double interval
    source << 12;
    intervals << ImapInterval(12, 12);
    QTest::newRow("double interval") << source << intervals << QByteArray("2:8,12");
}

void ImapSetTest::testAddList()
{
    QFETCH(QList<qint64>, source);
    QFETCH(ImapInterval::List, intervals);
    QFETCH(QByteArray, seqset);

    ImapSet set;
    set.add(source);
    ImapInterval::List result = set.intervals();
    QCOMPARE(result, intervals);
    QCOMPARE(set.toImapSequenceSet(), seqset);
}
