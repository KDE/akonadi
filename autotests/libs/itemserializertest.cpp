/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemserializertest.h"

#include "attributefactory.h"
#include "item.h"
#include "itemserializer_p.h"

#include <QTest>

#include <QStandardPaths>

using namespace Akonadi;

QTEST_MAIN(ItemSerializerTest)

void ItemSerializerTest::testEmptyPayload()
{
    // should not crash
    QByteArray data;
    Item item;
    ItemSerializer::deserialize(item, Item::FullPayload, data, 0, ItemSerializer::Internal);
    QVERIFY(data.isEmpty());
}

void ItemSerializerTest::testDefaultSerializer_data()
{
    QTest::addColumn<QByteArray>("serialized");

    QTest::newRow("null") << QByteArray();
    QTest::newRow("empty") << QByteArray("");
    QTest::newRow("nullbytei") << QByteArray("\0", 1);
    QTest::newRow("mixed") << QByteArray("\0\r\n\0bla", 7);
}

void ItemSerializerTest::testDefaultSerializer()
{
    // Avoid ItemSerializer from picking config from user's config, which affects how/whether
    // compresion is enabled or not.
    QStandardPaths::setTestModeEnabled(true);

    QFETCH(QByteArray, serialized);
    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));
    ItemSerializer::deserialize(item, Item::FullPayload, serialized, 0, ItemSerializer::Internal);

    QVERIFY(item.hasPayload<QByteArray>());
    QCOMPARE(item.payload<QByteArray>(), serialized);

    QByteArray data;
    int version = 0;
    ItemSerializer::serialize(item, Item::FullPayload, data, version);
    QCOMPARE(data, serialized);
    QEXPECT_FAIL("null", "Serializer cannot distinguish null vs. empty", Continue);
    QCOMPARE(data.isNull(), serialized.isNull());
}
