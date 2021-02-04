/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitydisplayattribute.h"

#include <QObject>

#include <QTest>

using namespace Akonadi;

class EntityDisplayAttributeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDeserialize_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("icon");
        QTest::addColumn<QString>("activeIcon");
        QTest::addColumn<QByteArray>("output");

        QTest::newRow("empty") << QByteArray("(\"\" \"\")") << QString() << QString() << QString() << QByteArray("(\"\" \"\" \"\" ())");
        QTest::newRow("name+icon") << QByteArray("(\"name\" \"icon\")") << QStringLiteral("name") << QStringLiteral("icon") << QString()
                                   << QByteArray("(\"name\" \"icon\" \"\" ())");
        QTest::newRow("name+icon+activeIcon") << QByteArray("(\"name\" \"icon\" \"activeIcon\")") << QStringLiteral("name") << QStringLiteral("icon")
                                              << QStringLiteral("activeIcon") << QByteArray("(\"name\" \"icon\" \"activeIcon\" ())");
    }

    void testDeserialize()
    {
        QFETCH(QByteArray, input);
        QFETCH(QString, name);
        QFETCH(QString, icon);
        QFETCH(QString, activeIcon);
        QFETCH(QByteArray, output);

        auto attr = new EntityDisplayAttribute();
        attr->deserialize(input);
        QCOMPARE(attr->displayName(), name);
        QCOMPARE(attr->iconName(), icon);
        QCOMPARE(attr->activeIconName(), activeIcon);

        QCOMPARE(attr->serialized(), output);

        EntityDisplayAttribute *copy = attr->clone();
        QCOMPARE(copy->serialized(), output);

        delete attr;
        delete copy;
    }
};

QTEST_MAIN(EntityDisplayAttributeTest)

#include "entitydisplayattributetest.moc"
