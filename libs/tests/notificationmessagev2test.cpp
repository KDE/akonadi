/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
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

#include "notificationmessagev2test.h"
#include <notificationmessagev2_p.h>

#include <QSet>
#include <QtTest/QTest>

QTEST_APPLESS_MAIN(NotificationMessageV2Test)

using namespace Akonadi;

void NotificationMessageV2Test::testCompress()
{
    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::Add);

    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(NotificationMessageV2::Modify);
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessageV2::Add);

    msg.setOperation(NotificationMessageV2::Remove);
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 2);   // should be 2 for collections, 0 for items?
}

void NotificationMessageV2Test::testCompress2()
{
    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::Modify);

    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(NotificationMessageV2::Remove);
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessageV2::Remove);
}

void NotificationMessageV2Test::testCompress3()
{
    NotificationMessageV2::List list;

    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::Modify);
    msg.setItemParts(QSet<QByteArray>() << "PART1");
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    NotificationMessageV2 msg2;
    msg2.setType(NotificationMessageV2::Items);
    msg2.setOperation(NotificationMessageV2::Modify);
    msg2.setItemParts(QSet<QByteArray>() << "PART2");
    NotificationMessageV2::appendAndCompress(list, msg2);

    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().itemParts(), QSet<QByteArray>() << "PART1" << "PART2");
}

void NotificationMessageV2Test::testCompress4()
{
    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::ModifyFlags);

    QSet<QByteArray> set;
    set << "FLAG1";
    msg.setAddedFlags(set);
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    set.clear();
    msg.setAddedFlags(set);
    set << "FLAG2";
    msg.setRemovedFlags(set);
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().addedFlags(), (QSet<QByteArray>() << "FLAG1"));
    QCOMPARE(list.first().removedFlags(), (QSet<QByteArray>() << "FLAG2"));
}

void NotificationMessageV2Test::testCompress5()
{
    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::ModifyFlags);

    msg.setAddedFlags(QSet<QByteArray>() << "FLAG1" << "FLAG2");
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    NotificationMessageV2 msg2;
    msg2.setType(NotificationMessageV2::Items);
    msg2.setOperation(NotificationMessageV2::Add);
    msg2.setAddedFlags(QSet<QByteArray>());
    NotificationMessageV2::appendAndCompress(list, msg2);

    msg.setAddedFlags(QSet<QByteArray>());
    msg.setRemovedFlags(QSet<QByteArray>() << "FLAG2" << "FLAG1");
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessageV2::Add);
}

void NotificationMessageV2Test::testCompress6()
{
    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::ModifyFlags);
    msg.setAddedFlags(QSet<QByteArray>() << "FLAG1");
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    NotificationMessageV2 msg2;
    msg2.setType(NotificationMessageV2::Items);
    msg2.setOperation(NotificationMessageV2::ModifyFlags);
    msg2.setAddedFlags(QSet<QByteArray>() << "FLAG2");
    NotificationMessageV2::appendAndCompress(list, msg2);

    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessageV2::ModifyFlags);
    QCOMPARE(list.first().addedFlags(), QSet<QByteArray>() << "FLAG1" << "FLAG2");
}

void NotificationMessageV2Test::testCompress7()
{
    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::Modify);
    msg.setItemParts(QSet<QByteArray>() << "PART1");
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    NotificationMessageV2 msg2;
    msg2.setType(NotificationMessageV2::Items);
    msg2.setOperation(NotificationMessageV2::ModifyFlags);
    msg2.setAddedFlags(QSet<QByteArray>() << "FLAG1");
    NotificationMessageV2::appendAndCompress(list, msg2);

    QCOMPARE(list.count(), 2);
}

void NotificationMessageV2Test::testCompressWithItemParts()
{
    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::Add);

    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(NotificationMessageV2::Modify);
    QSet<QByteArray> parts;
    parts << "SomePart";
    msg.setAddedFlags(parts);
    NotificationMessageV2::appendAndCompress(list, msg);
    NotificationMessageV2::appendAndCompress(list, msg);

    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessageV2::Add);
    QCOMPARE(list.first().itemParts(), QSet<QByteArray>());

    list.clear();
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(NotificationMessageV2::Remove);
    msg.setItemParts(QSet<QByteArray>());
    NotificationMessageV2::appendAndCompress(list, msg);

    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessageV2::Remove);
    QCOMPARE(list.first().itemParts(), QSet<QByteArray>());
}

void NotificationMessageV2Test::testNoCompress()
{
    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(NotificationMessageV2::Items);
    msg.setOperation(NotificationMessageV2::Modify);

    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setType(NotificationMessageV2::Collections);
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 2);
}

void NotificationMessageV2Test::testPartModificationMerge_data()
{
    QTest::addColumn<NotificationMessageV2::Type>("type");
    QTest::newRow("item") << NotificationMessageV2::Items;
    QTest::newRow("collection") << NotificationMessageV2::Collections;
}

void NotificationMessageV2Test::testPartModificationMerge()
{
    QFETCH(NotificationMessageV2::Type, type);

    NotificationMessageV2::List list;
    NotificationMessageV2 msg;
    msg.setType(type);
    msg.setOperation(NotificationMessageV2::Modify);
    msg.setItemParts(QSet<QByteArray>() << "PART1");

    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setItemParts(QSet<QByteArray>() << "PART2");
    NotificationMessageV2::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().itemParts(), (QSet<QByteArray>() << "PART1" << "PART2"));
}
