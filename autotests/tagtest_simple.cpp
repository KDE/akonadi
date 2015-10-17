/*
    Copyright (c) 2015 Volker Krause <vkrause@kde.org>

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

#include <QObject>
#include <qtest_akonadi.h>
#include "testattribute.h"

#include <attributefactory.h>
#include <tag.h>
#include <tagattribute.h>

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

#include "tagtest_simple.moc"

QTEST_MAIN(TagTestSimple)
