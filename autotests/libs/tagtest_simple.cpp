/*
    SPDX-FileCopyrightText: 2015 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include "attributefactory.h"
#include "qtest_akonadi.h"
#include "tag.h"
#include "tagattribute.h"
#include "testattribute.h"

using namespace Akonadi;

// Tag tests not requiring a full Akonadi test environment
// this is mainly to test memory management of attributes, so this is best used with valgrind/ASan
class TagTestSimple : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCustomAttributes();
    void testTagAttribute();
    void testDetach();
};

void TagTestSimple::testCustomAttributes()
{
    Tag t2;
    {
        Tag t1;
        auto attr = new TestAttribute;
        attr->deserialize("hello");
        t1.addAttribute(attr);
        t2 = t1;
    }
    QVERIFY(t2.hasAttribute("EXTRA"));
    auto attr = t2.attribute<TestAttribute>();
    QCOMPARE(attr->serialized(), QByteArray("hello"));
}

void TagTestSimple::testTagAttribute()
{
    Tag t2;
    {
        Tag t1;
        auto attr = AttributeFactory::createAttribute("TAG");
        t1.addAttribute(attr);
        t1.setName(QStringLiteral("hello"));
        t2 = t1;
    }
    QVERIFY(t2.hasAttribute<TagAttribute>());
    auto attr = t2.attribute<TagAttribute>();
    QVERIFY(attr);
    QCOMPARE(t2.name(), attr->displayName());
}

void TagTestSimple::testDetach()
{
    // Have a tag
    Tag tag;
    tag.setName(QStringLiteral("TAG"));
    auto origAttr = new TestAttribute();
    origAttr->data = "TEST";
    tag.addAttribute(origAttr);

    // Create a non-const copy so that it detaches on non-const access
    Tag copy = tag;

    // Do a non-const access to detach the copy
    auto attr = copy.attribute<TestAttribute>(Akonadi::Tag::AddIfMissing);
    auto attr2 = copy.attribute<TestAttribute>();

    // Verify that the detached attributes are the same
    QCOMPARE(attr, attr2);
    QCOMPARE(attr->data, origAttr->data);

    // But that their are a copy from the original attribute
    QVERIFY(attr != origAttr);
}

#include "tagtest_simple.moc"

QTEST_GUILESS_MAIN(TagTestSimple)
