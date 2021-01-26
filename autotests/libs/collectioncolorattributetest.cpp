/*
    SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectioncolorattribute.h"

#include <QObject>

#include <QTest>

using namespace Akonadi;

class CollectionColorAttributeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDeserialize_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QColor>("color");
        QTest::addColumn<QByteArray>("output");

        QTest::newRow("empty") << QByteArray("") << QColor() << QByteArray("");
        QTest::newRow("white") << QByteArray("white") << QColor("#ffffff") << QByteArray("#ffffffff");
        QTest::newRow("#123") << QByteArray("#123") << QColor("#112233") << QByteArray("#ff112233");
        QTest::newRow("#123456") << QByteArray("#123456") << QColor("#123456") << QByteArray("#ff123456");
        QTest::newRow("#1234567") << QByteArray("#1234567") << QColor() << QByteArray("");
        QTest::newRow("#12345678") << QByteArray("#12345678") << QColor("#12345678") << QByteArray("#12345678");
        QTest::newRow("#ff345678") << QByteArray("#ff123456") << QColor("#123456") << QByteArray("#ff123456");
    }

    void testDeserialize()
    {
        QFETCH(QByteArray, input);
        QFETCH(QColor, color);
        QFETCH(QByteArray, output);

        auto *attr = new CollectionColorAttribute();
        attr->deserialize(input);
        QCOMPARE(attr->color(), color);

        QCOMPARE(attr->serialized(), output);

        CollectionColorAttribute *copy = attr->clone();
        QCOMPARE(copy->serialized(), output);

        delete attr;
        delete copy;
    }
};

QTEST_MAIN(CollectionColorAttributeTest)

#include "collectioncolorattributetest.moc"
