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

#include "collectionattributetest.h"
#include "collectionpathresolver.h"

#include <collection.h>
#include <attributefactory.h>
#include <collectioncreatejob.h>
#include <collectiondeletejob.h>
#include <collectionfetchjob.h>
#include <collectionmodifyjob.h>
#include <collectionrightsattribute_p.h>
#include <control.h>
#include <collectionidentificationattribute.h>

#include <qtest_akonadi.h>

using namespace Akonadi;

QTEST_AKONADIMAIN(CollectionAttributeTest)

class TestAttribute : public Attribute
{
public:
    TestAttribute()
        : Attribute()
    {
    }
    TestAttribute(const QByteArray &data)
        : mData(data)
    {
    }
    TestAttribute *clone() const Q_DECL_OVERRIDE
    {
        return new TestAttribute(mData);
    }
    QByteArray type() const Q_DECL_OVERRIDE
    {
        return "TESTATTRIBUTE";
    }
    QByteArray serialized() const Q_DECL_OVERRIDE
    {
        return mData;
    }
    void deserialize(const QByteArray &data) Q_DECL_OVERRIDE
    {
        mData = data;
    }
private:
    QByteArray mData;
};

static int parentColId = -1;

void CollectionAttributeTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    Control::start();
    AttributeFactory::registerAttribute<TestAttribute>();

    CollectionPathResolver *resolver = new CollectionPathResolver(QStringLiteral("res3"), this);
    AKVERIFYEXEC(resolver);
    parentColId = resolver->collection();
    QVERIFY(parentColId > 0);
}

void CollectionAttributeTest::testAttributes_data()
{
    QTest::addColumn<QByteArray>("attr1");
    QTest::addColumn<QByteArray>("attr2");

    QTest::newRow("basic") << QByteArray("foo") << QByteArray("bar");
    QTest::newRow("empty1") << QByteArray("") << QByteArray("non-empty");
    QTest::newRow("empty2") << QByteArray("non-empty") << QByteArray("");
    QTest::newRow("space") << QByteArray("foo bar") << QByteArray("bar foo");
    QTest::newRow("newline") << QByteArray("\n") << QByteArray("no newline");
    QTest::newRow("newline2") << QByteArray(" \\\n\\\nnn") << QByteArray("no newline");
    QTest::newRow("cr") << QByteArray("\r") << QByteArray("\\\r\n");
    QTest::newRow("quotes") << QByteArray("\"quoted \\ test\"") << QByteArray("single \" quote \\");
    QTest::newRow("parenthesis") << QByteArray(")") << QByteArray("(");
    QTest::newRow("binary") << QByteArray("\000") << QByteArray("\001");
}

void CollectionAttributeTest::testAttributes()
{
    QFETCH(QByteArray, attr1);
    QFETCH(QByteArray, attr2);

    // add a custom attribute
    TestAttribute *attr = new TestAttribute();
    attr->deserialize(attr1);
    Collection col;
    col.setName(QStringLiteral("attribute test"));
    col.setParentCollection(Collection(parentColId));
    col.addAttribute(attr);
    CollectionCreateJob *create = new CollectionCreateJob(col, this);
    AKVERIFYEXEC(create);
    col = create->collection();
    QVERIFY(col.isValid());

    attr = col.attribute<TestAttribute>();
    QVERIFY(attr != 0);
    QCOMPARE(attr->serialized(), QByteArray(attr1));

    CollectionFetchJob *list = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(list);
    QCOMPARE(list->collections().count(), 1);
    col = list->collections().first();

    QVERIFY(col.isValid());
    attr = col.attribute<TestAttribute>();
    QVERIFY(attr != 0);
    QCOMPARE(attr->serialized(), QByteArray(attr1));

    TestAttribute *attrB = new TestAttribute();
    attrB->deserialize(attr2);
    col.addAttribute(attrB);
    attrB = col.attribute<TestAttribute>();
    QVERIFY(attrB != 0);
    QCOMPARE(attrB->serialized(), QByteArray(attr2));

    attrB->deserialize(attr1);
    col.addAttribute(attrB);
    attrB = col.attribute<TestAttribute>();
    QVERIFY(attrB != 0);
    QCOMPARE(attrB->serialized(), QByteArray(attr1));

    // modify a custom attribute
    col.attribute<TestAttribute>(Collection::AddIfMissing)->deserialize(attr2);
    CollectionModifyJob *modify = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(modify);

    list = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(list);
    QCOMPARE(list->collections().count(), 1);
    col = list->collections().first();

    QVERIFY(col.isValid());
    attr = col.attribute<TestAttribute>();
    QVERIFY(attr != 0);
    QCOMPARE(attr->serialized(), QByteArray(attr2));

    // delete a custom attribute
    col.removeAttribute<TestAttribute>();
    modify = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(modify);

    list = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(list);
    QCOMPARE(list->collections().count(), 1);
    col = list->collections().first();

    QVERIFY(col.isValid());
    attr = col.attribute<TestAttribute>();
    QVERIFY(attr == 0);

    // cleanup
    CollectionDeleteJob *del = new CollectionDeleteJob(col, this);
    AKVERIFYEXEC(del);

}

void CollectionAttributeTest::testDefaultAttributes()
{
    Collection col;
    QCOMPARE(col.attributes().count(), 0);
    Attribute *attr = AttributeFactory::createAttribute("TYPE");
    QVERIFY(attr);
    attr->deserialize("VALUE");
    col.addAttribute(attr);
    QCOMPARE(col.attributes().count(), 1);
    QVERIFY(col.hasAttribute("TYPE"));
    QCOMPARE(col.attribute("TYPE")->serialized(), QByteArray("VALUE"));
}

void CollectionAttributeTest::testCollectionRightsAttribute()
{
    CollectionRightsAttribute attribute;
    Collection::Rights rights;

    QCOMPARE(attribute.rights(), rights);

    for (int mask = 0; mask <= Collection::AllRights; ++mask) {
        rights = Collection::AllRights;
        rights &= mask;
        QCOMPARE(rights, mask);

        attribute.setRights(rights);
        QCOMPARE(attribute.rights(), rights);

        QByteArray data = attribute.serialized();
        attribute.deserialize(data);
        QCOMPARE(attribute.rights(), rights);
    }
}

void CollectionAttributeTest::testCollectionIdentifcationAttribute()
{
    QByteArray id("identifier");
    QByteArray ns("namespace");
    CollectionIdentificationAttribute attribute(id, ns);
    QCOMPARE(attribute.identifier(), id);
    QCOMPARE(attribute.collectionNamespace(), ns);

    QByteArray result = attribute.serialized();
    CollectionIdentificationAttribute parsed;
    parsed.deserialize(result);
    qDebug() << parsed.identifier() << parsed.collectionNamespace() << result;;
    QCOMPARE(parsed.identifier(), id);
    QCOMPARE(parsed.collectionNamespace(), ns);
}
