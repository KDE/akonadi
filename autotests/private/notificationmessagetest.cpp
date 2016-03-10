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
    CollectionChangeNotification msg;
    msg.setOperation(CollectionChangeNotification::Add);

    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(CollectionChangeNotification::Modify);
    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(static_cast<CollectionChangeNotification&>(list.first()).operation(), CollectionChangeNotification::Add);

    msg.setOperation(CollectionChangeNotification::Remove);
    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 2);
}

void NotificationMessageTest::testCompress2()
{
    ChangeNotification::List list;
    CollectionChangeNotification msg;
    msg.setOperation(CollectionChangeNotification::Modify);

    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setOperation(CollectionChangeNotification::Remove);
    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 2);
    QCOMPARE(static_cast<CollectionChangeNotification&>(list.first()).operation(), CollectionChangeNotification::Modify);
    QCOMPARE(static_cast<CollectionChangeNotification&>(list.last()).operation(), CollectionChangeNotification::Remove);
}

void NotificationMessageTest::testCompress3()
{
    ChangeNotification::List list;
    CollectionChangeNotification msg;
    msg.setOperation(CollectionChangeNotification::Modify);

    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
}

void NotificationMessageTest::testPartModificationMerge()
{
    ChangeNotification::List list;
    CollectionChangeNotification msg;
    msg.setOperation(CollectionChangeNotification::Modify);
    msg.setChangedParts(QSet<QByteArray>() << "PART1");

    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);

    msg.setChangedParts(QSet<QByteArray>() << "PART2");
    CollectionChangeNotification::appendAndCompress(list, msg);
    QCOMPARE(list.count(), 1);
    QCOMPARE(static_cast<CollectionChangeNotification&>(list.first()).changedParts(), (QSet<QByteArray>() << "PART1" << "PART2"));
}
