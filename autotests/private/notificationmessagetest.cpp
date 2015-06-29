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
#include <private/protocol_p.h>

#include <QSet>
#include <QtTest/QTest>

QTEST_APPLESS_MAIN(NotificationMessageTest)

using namespace Akonadi;
using namespace Akonadi::Protocol;

void NotificationMessageTest::testCompress()
{
    ChangeNotification::List list;
    ChangeNotification msg;
    msg.setType(ChangeNotification::Collections);
    msg.setOperation(ChangeNotification::Add);

    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(ChangeNotification::Modify);
    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().operation(), ChangeNotification::Add);

    msg.setOperation(ChangeNotification::Remove);
    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 2);
}

void NotificationMessageTest::testCompress2()
{
    ChangeNotification::List list;
    ChangeNotification msg;
    msg.setType(ChangeNotification::Collections);
    msg.setOperation(ChangeNotification::Modify);

    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(ChangeNotification::Remove);
    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 2);
    QCOMPARE(list.first().operation(), ChangeNotification::Modify);
    QCOMPARE(list.last().operation(), ChangeNotification::Remove);
}

void NotificationMessageTest::testCompress3()
{
    ChangeNotification::List list;
    ChangeNotification msg;
    msg.setType(ChangeNotification::Collections);
    msg.setOperation(ChangeNotification::Modify);

    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
}

void NotificationMessageTest::testPartModificationMerge_data()
{
    QTest::addColumn<ChangeNotification::Type>("type");
    QTest::newRow("collection") << ChangeNotification::Collections;
}

void NotificationMessageTest::testPartModificationMerge()
{
    QFETCH(ChangeNotification::Type, type);

    ChangeNotification::List list;
    ChangeNotification msg;
    msg.setType(type);
    msg.setOperation(ChangeNotification::Modify);
    msg.setItemParts(QSet<QByteArray>() << "PART1");

    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setItemParts(QSet<QByteArray>() << "PART2");
    ChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().itemParts(), (QSet<QByteArray>() << "PART1" << "PART2"));
}
