/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "test_utils.h"

#include <akonadi/control.h>
#include <akonadi/tagcreatejob.h>
#include <akonadi/tagfetchjob.h>
#include <akonadi/tagdeletejob.h>
#include <akonadi/tagattribute.h>
#include <akonadi/tagfetchscope.h>
#include <tagmodifyjob.h>
#include <resourceselectjob_p.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/attributefactory.h>

using namespace Akonadi;

class TagTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testCreateFetch();
    void testRID();
    void testDelete();
    void testModify();
    void testCreateMerge();
    void testAttributes();
    void testTagItem();
    void testMonitor();
};

void TagTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    AkonadiTest::setAllResourcesOffline();
    AttributeFactory::registerAttribute<TagAttribute>();
    qRegisterMetaType<Akonadi::Tag>();
    qRegisterMetaType<QSet<Akonadi::Tag> >();
    qRegisterMetaType<Akonadi::Item::List>();
}

void TagTest::testCreateFetch()
{
    Tag tag;
    tag.setGid("gid");
    tag.setType("mytype");
    TagCreateJob *createjob = new TagCreateJob(tag, this);
    AKVERIFYEXEC(createjob);
    QVERIFY(createjob->tag().isValid());

    {
        TagFetchJob *fetchJob = new TagFetchJob(this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QCOMPARE(fetchJob->tags().first().gid(), QByteArray("gid"));
        QCOMPARE(fetchJob->tags().first().type(), QByteArray("mytype"));

        TagDeleteJob *deleteJob = new TagDeleteJob(fetchJob->tags().first(), this);
        AKVERIFYEXEC(deleteJob);
    }

    {
        TagFetchJob *fetchJob = new TagFetchJob(this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 0);
    }
}

void TagTest::testRID()
{
    {
        ResourceSelectJob *select = new ResourceSelectJob("akonadi_knut_resource_0");
        AKVERIFYEXEC(select);
    }
    Tag tag;
    tag.setGid("gid");
    tag.setType("mytype");
    tag.setRemoteId("rid");
    TagCreateJob *createjob = new TagCreateJob(tag, this);
    AKVERIFYEXEC(createjob);
    QVERIFY(createjob->tag().isValid());

    {
        TagFetchJob *fetchJob = new TagFetchJob(this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QCOMPARE(fetchJob->tags().first().gid(), QByteArray("gid"));
        QCOMPARE(fetchJob->tags().first().type(), QByteArray("mytype"));
        QCOMPARE(fetchJob->tags().first().remoteId(), QByteArray("rid"));

        TagDeleteJob *deleteJob = new TagDeleteJob(fetchJob->tags().first(), this);
        AKVERIFYEXEC(deleteJob);
    }
    {
        ResourceSelectJob *select = new ResourceSelectJob("");
        AKVERIFYEXEC(select);
    }
}

void TagTest::testDelete()
{
    Tag tag1;
    {
      tag1.setGid("tag1");
      TagCreateJob *createjob = new TagCreateJob(tag1, this);
      AKVERIFYEXEC(createjob);
      QVERIFY(createjob->tag().isValid());
      tag1 = createjob->tag();
    }
    Tag tag2;
    {
      tag2.setGid("tag2");
      TagCreateJob *createjob = new TagCreateJob(tag2, this);
      AKVERIFYEXEC(createjob);
      QVERIFY(createjob->tag().isValid());
      tag2 = createjob->tag();
    }
    {
      TagDeleteJob *deleteJob = new TagDeleteJob(tag1, this);
      AKVERIFYEXEC(deleteJob);
    }

    {
        TagFetchJob *fetchJob = new TagFetchJob(this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QCOMPARE(fetchJob->tags().first().gid(), tag2.gid());
    }
    {
      TagDeleteJob *deleteJob = new TagDeleteJob(tag2, this);
      AKVERIFYEXEC(deleteJob);
    }
}

void TagTest::testModify()
{
    Tag tag;
    {
      tag.setGid("gid");
      TagCreateJob *createjob = new TagCreateJob(tag, this);
      AKVERIFYEXEC(createjob);
      QVERIFY(createjob->tag().isValid());
      tag = createjob->tag();
    }

    //We can add an attribute
    {
        Akonadi::TagAttribute *attr = tag.attribute<Akonadi::TagAttribute>(AttributeEntity::AddIfMissing);
        attr->setDisplayName("display name");
        tag.addAttribute(attr);
        tag.setParent(Tag(0));
        tag.setType("mytype");
        TagModifyJob *modJob = new TagModifyJob(tag, this);
        AKVERIFYEXEC(modJob);

        TagFetchJob *fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QVERIFY(fetchJob->tags().first().hasAttribute<Akonadi::TagAttribute>());
    }
    //We can update an attribute
    {
        Akonadi::TagAttribute *attr = tag.attribute<Akonadi::TagAttribute>(AttributeEntity::AddIfMissing);
        attr->setDisplayName("display name2");
        TagModifyJob *modJob = new TagModifyJob(tag, this);
        AKVERIFYEXEC(modJob);

        TagFetchJob *fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QVERIFY(fetchJob->tags().first().hasAttribute<Akonadi::TagAttribute>());
        QCOMPARE(fetchJob->tags().first().attribute<Akonadi::TagAttribute>()->displayName(), attr->displayName());
    }
    //We can clear an attribute
    {
        tag.removeAttribute<Akonadi::TagAttribute>();
        TagModifyJob *modJob = new TagModifyJob(tag, this);
        AKVERIFYEXEC(modJob);

        TagFetchJob *fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QVERIFY(!fetchJob->tags().first().hasAttribute<Akonadi::TagAttribute>());
    }

    TagDeleteJob *deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testCreateMerge()
{
    Tag tag;
    {
      tag.setGid("gid");
      TagCreateJob *createjob = new TagCreateJob(tag, this);
      AKVERIFYEXEC(createjob);
      QVERIFY(createjob->tag().isValid());
      tag = createjob->tag();
    }
    {
      Tag tag2;
      tag2.setGid("gid");
      TagCreateJob *createjob = new TagCreateJob(tag2, this);
      createjob->setMergeIfExisting( true );
      AKVERIFYEXEC(createjob);
      QVERIFY(createjob->tag().isValid());
      QCOMPARE(createjob->tag().id(), tag.id());
    }

    TagDeleteJob *deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testAttributes()
{
    Tag tag;
    {
      tag.setGid("gid2");
      TagAttribute *attr = tag.attribute<TagAttribute>(AttributeEntity::AddIfMissing);
      attr->setDisplayName("name");
      attr->setInToolbar(true);
      tag.addAttribute(attr);
      TagCreateJob *createjob = new TagCreateJob(tag, this);
      AKVERIFYEXEC(createjob);
      QVERIFY(createjob->tag().isValid());
      tag = createjob->tag();

      {
          TagFetchJob *fetchJob = new TagFetchJob(createjob->tag(), this);
          fetchJob->fetchScope().fetchAttribute<TagAttribute>();
          AKVERIFYEXEC(fetchJob);
          QCOMPARE(fetchJob->tags().size(), 1);
          QVERIFY(fetchJob->tags().first().hasAttribute<TagAttribute>());
          //we need to clone because the returned attribute is just a reference and destroyed on the next line
          //FIXME we should find a better solution for this (like returning a smart pointer or value object)
          QScopedPointer<TagAttribute> tagAttr(fetchJob->tags().first().attribute<TagAttribute>()->clone());
          QVERIFY(tagAttr);
          QCOMPARE(tagAttr->displayName(), QStringLiteral("name"));
          QCOMPARE(tagAttr->inToolbar(), true);
      }
    }
    //Try fetching multiple items
    Tag tag2;
    {
      tag2.setGid("gid22");
      TagAttribute *attr = tag.attribute<TagAttribute>(AttributeEntity::AddIfMissing);
      attr->setDisplayName("name2");
      attr->setInToolbar(true);
      tag2.addAttribute(attr);
      TagCreateJob *createjob = new TagCreateJob(tag2, this);
      AKVERIFYEXEC(createjob);
      QVERIFY(createjob->tag().isValid());
      tag2 = createjob->tag();

      {
          TagFetchJob *fetchJob = new TagFetchJob(Tag::List() << tag << tag2, this);
          fetchJob->fetchScope().fetchAttribute<TagAttribute>();
          AKVERIFYEXEC(fetchJob);
          QCOMPARE(fetchJob->tags().size(), 2);
          QVERIFY(fetchJob->tags().at(0).hasAttribute<TagAttribute>());
          QVERIFY(fetchJob->tags().at(1).hasAttribute<TagAttribute>());
      }
    }

    TagDeleteJob *deleteJob = new TagDeleteJob(Tag::List() << tag << tag2, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testTagItem()
{
    Akonadi::Monitor monitor;
    monitor.itemFetchScope().setFetchTags(true);
    monitor.setAllMonitored(true);
    const Collection res3 = Collection( collectionIdFromPath( "res3" ) );
    Tag tag;
    {
        TagCreateJob *createjob = new TagCreateJob(Tag("gid1"), this);
        AKVERIFYEXEC(createjob);
        tag = createjob->tag();
    }

    Item item1;
    {
        item1.setMimeType( "application/octet-stream" );
        ItemCreateJob *append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }

    item1.setTag(tag);

    QSignalSpy tagsSpy(&monitor, SIGNAL(itemsTagsChanged(Akonadi::Item::List,QSet<Akonadi::Tag>,QSet<Akonadi::Tag>)));
    QVERIFY(tagsSpy.isValid());

    ItemModifyJob *modJob = new ItemModifyJob(item1, this);
    AKVERIFYEXEC(modJob);

    QTRY_VERIFY(tagsSpy.count() >= 1);
    QTRY_COMPARE(tagsSpy.last().first().value<Akonadi::Item::List>().first().id(), item1.id());
    QTRY_COMPARE(tagsSpy.last().at(1).value< QSet<Tag> >().size(), 1); //1 added tag

    ItemFetchJob *fetchJob = new ItemFetchJob(item1, this);
    fetchJob->fetchScope().setFetchTags(true);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().first().tags().size(), 1);
}

void TagTest::testMonitor()
{
  Akonadi::Monitor monitor;
  monitor.setTypeMonitored(Akonadi::Monitor::Tags);
  monitor.tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();

  Akonadi::Tag createdTag;
  {
    QSignalSpy addedSpy(&monitor, SIGNAL(tagAdded(Akonadi::Tag)));
    QVERIFY(addedSpy.isValid());
    Tag tag("gid2");
    tag.attribute<Akonadi::TagAttribute>(AttributeEntity::AddIfMissing);
    QVERIFY(tag.hasAttribute<Akonadi::TagAttribute>());
    TagCreateJob *createjob = new TagCreateJob(tag, this);
    AKVERIFYEXEC(createjob);
    //We usually pick up signals from the previous tests as well (due to server-side notification caching)
    QTRY_VERIFY(addedSpy.count() >= 1);
    QTRY_COMPARE(addedSpy.last().first().value<Akonadi::Tag>().id(), createjob->tag().id());
    QVERIFY(addedSpy.last().first().value<Akonadi::Tag>().hasAttribute<Akonadi::TagAttribute>());
    createdTag = createjob->tag();
  }

  {
    QSignalSpy removedSpy(&monitor, SIGNAL(tagRemoved(Akonadi::Tag)));
    QVERIFY(removedSpy.isValid());
    TagDeleteJob *deletejob = new TagDeleteJob(createdTag, this);
    AKVERIFYEXEC(deletejob);
    QTRY_VERIFY(removedSpy.count() >= 1);
    QTRY_COMPARE(removedSpy.last().first().value<Akonadi::Tag>().id(), createdTag.id());
  }
}

#include "tagtest.moc"

QTEST_AKONADIMAIN(TagTest, NoGUI)
