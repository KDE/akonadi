/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "attributefactorytest.h"
#include "collectionpathresolver.h"
#include "testattribute.h"

#include <attributefactory.h>
#include <collection.h>
#include <itemcreatejob.h>
#include <itemfetchjob.h>
#include <itemfetchscope.h>
#include <resourceselectjob_p.h>
#include <qtest_akonadi.h>

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
    TestAttribute *ta = new TestAttribute;
    QVERIFY(AttributeFactory::createAttribute(ta->type()));     // DefaultAttribute
    ta->data = "lalala";
    item.addAttribute(ta);
    ItemCreateJob *cjob = new ItemCreateJob(item, res1);
    AKVERIFYEXEC(cjob);
    int id = cjob->item().id();
    item = Item(id);
    ItemFetchJob *fjob = new ItemFetchJob(item);
    fjob->fetchScope().fetchFullPayload();
    fjob->fetchScope().fetchAllAttributes();
    AKVERIFYEXEC(fjob);
    QCOMPARE(fjob->items().count(), 1);
    item = fjob->items().first();
    QVERIFY(item.hasAttribute<TestAttribute>());   // has DefaultAttribute
    ta = item.attribute<TestAttribute>();
    QVERIFY(!ta);   // but can't cast it to TestAttribute
}

void AttributeFactoryTest::testRegisteredAttribute()
{
    AttributeFactory::registerAttribute<TestAttribute>();

    Item item;
    item.setMimeType(QStringLiteral("text/directory"));
    item.setPayload<QByteArray>("payload");
    TestAttribute *ta = new TestAttribute;
    QVERIFY(AttributeFactory::createAttribute(ta->type()) != 0);
    ta->data = "lalala";
    item.addAttribute(ta);
    ItemCreateJob *cjob = new ItemCreateJob(item, res1);
    AKVERIFYEXEC(cjob);
    int id = cjob->item().id();
    item = Item(id);
    ItemFetchJob *fjob = new ItemFetchJob(item);
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
