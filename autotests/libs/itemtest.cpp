/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemtest.h"
#include "collection.h"
#include "item.h"
#include "testattribute.h"

#include <QTest>

#include <memory>

QTEST_GUILESS_MAIN(ItemTest)

using namespace Akonadi;

void ItemTest::testMultipart()
{
    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));

    QSet<QByteArray> parts;
    QCOMPARE(item.loadedPayloadParts(), parts);

    QByteArray bodyData = "bodydata";
    item.setPayload<QByteArray>(bodyData);
    parts << Item::FullPayload;
    QCOMPARE(item.loadedPayloadParts(), parts);
    QCOMPARE(item.payload<QByteArray>(), bodyData);

    QByteArray myData = "mypartdata";
    item.attribute<TestAttribute>(Item::AddIfMissing)->data = myData;

    QCOMPARE(item.loadedPayloadParts(), parts);
    QCOMPARE(item.attributes().count(), 1);
    QVERIFY(item.hasAttribute<TestAttribute>());
    QCOMPARE(item.attribute<TestAttribute>()->data, myData);
}

void ItemTest::testInheritance()
{
    Item a;

    a.setRemoteId(QStringLiteral("Hello World"));
    a.setSize(10);

    Item b(a);
    b.setFlag("\\send");
    QCOMPARE(b.remoteId(), QStringLiteral("Hello World"));
    QCOMPARE(b.size(), (qint64)10);
}

void ItemTest::testParentCollection()
{
    Item a;
    QVERIFY(!a.parentCollection().isValid());

    a.setParentCollection(Collection::root());
    QCOMPARE(a.parentCollection(), Collection::root());
    Item b = a;
    QCOMPARE(b.parentCollection(), Collection::root());

    Item c;
    c.parentCollection().setRemoteId(QStringLiteral("foo"));
    QCOMPARE(c.parentCollection().remoteId(), QStringLiteral("foo"));
    const Item d = c;
    QCOMPARE(d.parentCollection().remoteId(), QStringLiteral("foo"));

    const Item e;
    QVERIFY(!e.parentCollection().isValid());

    Collection col(5);
    Item f;
    f.setParentCollection(col);
    QCOMPARE(f.parentCollection(), col);
    Item g = f;
    QCOMPARE(g.parentCollection(), col);
    b = g;
    QCOMPARE(b.parentCollection(), col);
}

void ItemTest::testComparison_data()
{
    QTest::addColumn<Akonadi::Item>("itemA");
    QTest::addColumn<Akonadi::Item>("itemB");
    QTest::addColumn<bool>("match");

    QTest::newRow("both invalid, same invalid IDs") << Item(-10) << Item(-10) << true;
    QTest::newRow("both invalid, different invalid IDs") << Item(-11) << Item(-12) << true;
    QTest::newRow("one valid") << Item(1) << Item() << false;
    QTest::newRow("both valid, same IDs") << Item(2) << Item(2) << true;
    QTest::newRow("both valid, different IDs") << Item(3) << Item(4) << false;
}

void ItemTest::testComparison()
{
    QFETCH(Akonadi::Item, itemA);
    QFETCH(Akonadi::Item, itemB);
    QFETCH(bool, match);

    if (match) {
        QVERIFY(itemA == itemB);
        QVERIFY(!(itemA != itemB));
    } else {
        QVERIFY(itemA != itemB);
        QVERIFY(!(itemA == itemB));
    }
}

void ItemTest::testDetach()
{
    // Have an item
    Item item;
    item.setMimeType(QStringLiteral("application/octet-stream"));
    item.setFlags({"Flag1"});
    item.addAttribute(new TestAttribute());
    auto origAttr = item.attribute<TestAttribute>();
    origAttr->data = "TEST";

    // Make a copy
    Item copy = item;

    // Detach by calling non-const attribute. Make sure a new copy of the attibute is returned
    auto attr = copy.attribute<TestAttribute>(Akonadi::Item::AddIfMissing);
    auto attr2 = copy.attribute<TestAttribute>();

    // Verify that detaching returned a copy of the source attribute
    QCOMPARE(attr, attr2);
    // and that its content is the same as the original
    QCOMPARE(attr->data, origAttr->data);

    // Make sure the detached attribute is a new instance
    QVERIFY(attr != origAttr);
}

#include "moc_itemtest.cpp"
