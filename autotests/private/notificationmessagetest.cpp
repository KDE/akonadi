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
#include <QTest>

QTEST_APPLESS_MAIN(NotificationMessageTest)

using namespace Akonadi;
using namespace Akonadi::Protocol;

void NotificationMessageTest::testCompress()
{
    ChangeNotificationList list;
    auto collection = FetchCollectionsResponsePtr::create(1);
    CollectionChangeNotification msg;
    msg.setCollection(collection);
    msg.setOperation(CollectionChangeNotification::Add);

    QVERIFY(CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 1);

    msg.setOperation(CollectionChangeNotification::Modify);
    QVERIFY(!CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().staticCast<CollectionChangeNotification>()->operation(), CollectionChangeNotification::Add);

    msg.setOperation(CollectionChangeNotification::Remove);
    QVERIFY(CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 2);
}

void NotificationMessageTest::testCompress2()
{
    ChangeNotificationList list;
    auto collection = FetchCollectionsResponsePtr::create(1);
    CollectionChangeNotification msg;
    msg.setCollection(collection);
    msg.setOperation(CollectionChangeNotification::Modify);

    QVERIFY(CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 1);

    msg.setOperation(CollectionChangeNotification::Remove);
    QVERIFY(CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 2);
    QCOMPARE(list.first().staticCast<CollectionChangeNotification>()->operation(), CollectionChangeNotification::Modify);
    QCOMPARE(list.last().staticCast<CollectionChangeNotification>()->operation(), CollectionChangeNotification::Remove);
}

void NotificationMessageTest::testCompress3()
{
    ChangeNotificationList list;
    auto collection = FetchCollectionsResponsePtr::create(1);
    CollectionChangeNotification msg;
    msg.setCollection(collection);
    msg.setOperation(CollectionChangeNotification::Modify);

    QVERIFY(CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 1);

    QVERIFY(!CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 1);
}

void NotificationMessageTest::testPartModificationMerge()
{
    ChangeNotificationList list;
    auto collection = FetchCollectionsResponsePtr::create(1);
    CollectionChangeNotification msg;
    msg.setCollection(collection);
    msg.setOperation(CollectionChangeNotification::Modify);
    msg.setChangedParts(QSet<QByteArray>() << "PART1");

    QVERIFY(CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 1);

    msg.setChangedParts(QSet<QByteArray>() << "PART2");
    QVERIFY(!CollectionChangeNotification::appendAndCompress(
                    list, CollectionChangeNotificationPtr::create(msg)));
    QCOMPARE(list.count(), 1);
    QCOMPARE(list.first().staticCast<CollectionChangeNotification>()->changedParts(), (QSet<QByteArray>() << "PART1" << "PART2"));
}
