/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "attributefactorytest.h"
#include "collectionpathresolver.h"
#include "testattribute.h"

#include "attributefactory.h"
#include "collection.h"
#include "itemcreatejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "qtest_akonadi.h"
#include "resourceselectjob_p.h"

using namespace Akonadi;

QTEST_AKONADIMAIN(AttributeFactoryTest)

static Collection res1;

void AttributeFactoryTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    CollectionPathResolver *resolver = new CollectionPathResolver(QStringLiteral("res1"), this);
    AKVERIFYEXEC(resolver);
    res1 = Collection(resolver->collection());
}

void AttributeFactoryTest::testUnknownAttribute()
{
    // The attribute is currently not registered.
    Item item;
    item.setMimeType(QStringLiteral("text/directory"));
    item.setPayload<QByteArray>("payload");
    auto *ta = new TestAttribute;
    {
        auto *created = AttributeFactory::createAttribute(ta->type()); // DefaultAttribute
        QVERIFY(created != nullptr);
        delete created;
    }
    ta->data = "lalala";
    item.addAttribute(ta);
    auto *cjob = new ItemCreateJob(item, res1);
    AKVERIFYEXEC(cjob);
    int id = cjob->item().id();
    item = Item(id);
    auto *fjob = new ItemFetchJob(item);
    fjob->fetchScope().fetchFullPayload();
    fjob->fetchScope().fetchAllAttributes();
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items().first();
    QVERIFY(item.hasAttribute<TestAttribute>()); // has DefaultAttribute
    ta = item.attribute<TestAttribute>();
    QVERIFY(!ta); // but can't cast it to TestAttribute
}

void AttributeFactoryTest::testRegisteredAttribute()
{
    AttributeFactory::registerAttribute<TestAttribute>();

    Item item;
    item.setMimeType(QStringLiteral("text/directory"));
    item.setPayload<QByteArray>("payload");
    auto *ta = new TestAttribute;
    {
        auto *created = AttributeFactory::createAttribute(ta->type());
        QVERIFY(created != nullptr);
        delete created;
    }
    ta->data = "lalala";
    item.addAttribute(ta);
    auto *cjob = new ItemCreateJob(item, res1);
    AKVERIFYEXEC(cjob);
    int id = cjob->item().id();
    item = Item(id);
    auto *fjob = new ItemFetchJob(item);
    fjob->fetchScope().fetchFullPayload();
    fjob->fetchScope().fetchAllAttributes();
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items().first();
    QVERIFY(item.hasAttribute<TestAttribute>());
    ta = item.attribute<TestAttribute>();
    QVERIFY(ta);
    QCOMPARE(ta->data, QByteArray("lalala"));
}
