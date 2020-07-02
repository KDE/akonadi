/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    FetchCollectionsResponse collection(1);
    CollectionChangeNotification msg;
    msg.setCollection(std::move(collection));
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
    FetchCollectionsResponse collection(1);
    CollectionChangeNotification msg;
    msg.setCollection(std::move(collection));
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
    FetchCollectionsResponse collection(1);
    CollectionChangeNotification msg;
    msg.setCollection(std::move(collection));
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
    FetchCollectionsResponse collection(1);
    CollectionChangeNotification msg;
    msg.setCollection(std::move(collection));
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
