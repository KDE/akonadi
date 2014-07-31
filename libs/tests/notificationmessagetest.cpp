/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "notificationmessagetest.h"
#include <notificationmessage_p.h>

#include <QSet>
#include <QtTest/QTest>

QTEST_APPLESS_MAIN(NotificationMessageTest)

using namespace Akonadi;

Q_DECLARE_METATYPE(NotificationMessage::Type)

void NotificationMessageTest::testCompress()
{
    NotificationMessage::List list;
    NotificationMessage msg;
    msg.setType(NotificationMessage::Item);
    msg.setOperation(NotificationMessage::Add);

    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(NotificationMessage::Modify);
    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessage::Add);

    msg.setOperation(NotificationMessage::Remove);
    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 2);   // should be 2 for collections, 0 for items?
}

void NotificationMessageTest::testCompress2()
{
    NotificationMessage::List list;
    NotificationMessage msg;
    msg.setType(NotificationMessage::Item);
    msg.setOperation(NotificationMessage::Modify);

    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(NotificationMessage::Remove);
    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessage::Remove);
}

void NotificationMessageTest::testCompress3()
{
    NotificationMessage::List list;
    NotificationMessage msg;
    msg.setType(NotificationMessage::Item);
    msg.setOperation(NotificationMessage::Modify);

    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
}

void NotificationMessageTest::testCompressWithItemParts()
{
    NotificationMessage::List list;
    NotificationMessage msg;
    msg.setType(NotificationMessage::Item);
    msg.setOperation(NotificationMessage::Add);

    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(NotificationMessage::Modify);
    QSet<QByteArray> changes;
    changes.insert("FLAGS");
    msg.setItemParts(changes);
    NotificationMessage::appendAndCompress(list, msg);
    NotificationMessage::appendAndCompress(list, msg);

    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessage::Add);
    QCOMPARE(list.first().itemParts(), QSet<QByteArray>());

    list.clear();
    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(NotificationMessage::Remove);
    msg.setItemParts(QSet<QByteArray>());
    NotificationMessage::appendAndCompress(list, msg);

    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), NotificationMessage::Remove);
    QCOMPARE(list.first().itemParts(), QSet<QByteArray>());
}

void NotificationMessageTest::testNoCompress()
{
    NotificationMessage::List list;
    NotificationMessage msg;
    msg.setType(NotificationMessage::Item);
    msg.setOperation(NotificationMessage::Modify);

    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setType(NotificationMessage::Collection);
    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 2);
}

void NotificationMessageTest::testPartModificationMerge_data()
{
    QTest::addColumn<NotificationMessage::Type>("type");
    QTest::newRow("item") << NotificationMessage::Item;
    QTest::newRow("collection") << NotificationMessage::Collection;
}

void NotificationMessageTest::testPartModificationMerge()
{
    QFETCH(NotificationMessage::Type, type);

    NotificationMessage::List list;
    NotificationMessage msg;
    msg.setType(type);
    msg.setOperation(NotificationMessage::Modify);
    msg.setItemParts(QSet<QByteArray>() << "PART1");

    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setItemParts(QSet<QByteArray>() << "PART2");
    NotificationMessage::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().itemParts(), (QSet<QByteArray>() << "PART1" << "PART2"));
}
