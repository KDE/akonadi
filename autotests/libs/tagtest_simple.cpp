/*
    SPDX-FileCopyrightText: 2015 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

#include "qtest_akonadi.h"
#include "testattribute.h"
#include "attributefactory.h"
#include "tag.h"
#include "tagattribute.h"

using namespace Akonadi;

// Tag tests not requiring a full Akonadi test environment
// this is mainly to test memory management of attributes, so this is best used with valgrind/ASan
class TagTestSimple : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCustomAttributes();
    void testTagAttribute();
};

void TagTestSimple::testCustomAttributes()
{
    Tag t2;
    {
        Tag t1;
        auto *attr = new TestAttribute;
        attr->deserialize("hello");
        t1.addAttribute(attr);
        t2 = t1;
    }
    QVERIFY(t2.hasAttribute("EXTRA"));
    auto *attr = t2.attribute<TestAttribute>();
    QCOMPARE(attr->serialized(), QByteArray("hello"));
}

void TagTestSimple::testTagAttribute()
{
    Tag t2;
    {
        Tag t1;
        auto *attr = AttributeFactory::createAttribute("TAG");
        t1.addAttribute(attr);
        t1.setName(QStringLiteral("hello"));
        t2 = t1;
    }
    QVERIFY(t2.hasAttribute<TagAttribute>());
    auto *attr = t2.attribute<TagAttribute>();
    QVERIFY(attr);
    QCOMPARE(t2.name(), attr->displayName());
}

#include "tagtest_simple.moc"

QTEST_MAIN(TagTestSimple)
