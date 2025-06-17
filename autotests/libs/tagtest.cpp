/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "attributefactory.h"
#include "control.h"
#include "item.h"
#include "itemcreatejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "monitor.h"
#include "qtest_akonadi.h"
#include "resourceselectjob_p.h"
#include "tagattribute.h"
#include "tagcreatejob.h"
#include "tagdeletejob.h"
#include "tagfetchjob.h"
#include "tagfetchscope.h"
#include "tagmodifyjob.h"

#include <QSignalSpy>

using namespace Akonadi;

class TagTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testTag();
    void testCreateFetch();
    void testRID();
    void testDelete();
    void testDeleteRIDIsolation();
    void testModify();
    void testModifyFromResource();
    void testCreateMerge();
    void testAttributes();
    void testTagItem();
    void testCreateItem();
    void testCreateItemWithTags();
    void testRIDIsolation();
    void testFetchTagIdWithItem();
    void testFetchFullTagWithItem();
    void testModifyItemWithTagByGID();
    void testModifyItemWithTagByRID();
    void testMonitor();
    void testTagAttributeConfusionBug();
    void testFetchItemsByTag();
    void tagModifyJobShouldOnlySendModifiedAttributes();
    void testItemNotificationOnTagDeletion();
};

void TagTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();
    AkonadiTest::setAllResourcesOffline();
    AttributeFactory::registerAttribute<TagAttribute>();
    qRegisterMetaType<Akonadi::Tag>();
    qRegisterMetaType<QSet<Akonadi::Tag>>();
    qRegisterMetaType<Akonadi::Item::List>();

    // Delete the default Knut tag - it's interfering with this test
    auto fetchJob = new TagFetchJob(this);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->tags().size(), 1);
    auto deleteJob = new TagDeleteJob(fetchJob->tags().first(), this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testTag()
{
    Tag tag1;
    Tag tag2;

    // Invalid tags are equal
    QVERIFY(tag1 == tag2);

    // Invalid tags with different GIDs are not equal
    tag1.setGid("GID1");
    QVERIFY(tag1 != tag2);
    tag2.setGid("GID2");
    QVERIFY(tag1 != tag2);

    // Invalid tags with equal GIDs are equal
    tag1.setGid("GID2");
    QVERIFY(tag1 == tag2);

    // Valid tags with different IDs are not equal
    tag1 = Tag(1);
    tag2 = Tag(2);
    QVERIFY(tag1 != tag2);

    // Valid tags with different IDs and equal GIDs are still not equal
    tag1.setGid("GID1");
    tag2.setGid("GID1");
    QVERIFY(tag1 != tag2);

    // Valid tags with equal ID are equal regardless of GIDs
    tag2 = Tag(1);
    tag2.setGid("GID2");
    QVERIFY(tag1 == tag2);
}

void TagTest::testCreateFetch()
{
    Tag tag;
    tag.setGid("gid");
    tag.setType("mytype");
    auto createjob = new TagCreateJob(tag, this);
    AKVERIFYEXEC(createjob);
    QVERIFY(createjob->tag().isValid());

    {
        auto fetchJob = new TagFetchJob(this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QCOMPARE(fetchJob->tags().first().gid(), QByteArray("gid"));
        QCOMPARE(fetchJob->tags().first().type(), QByteArray("mytype"));

        auto deleteJob = new TagDeleteJob(fetchJob->tags().first(), this);
        AKVERIFYEXEC(deleteJob);
    }

    {
        auto fetchJob = new TagFetchJob(this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 0);
    }
}

void TagTest::testRID()
{
    {
        auto select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);
    }
    Tag tag;
    tag.setGid("gid");
    tag.setType("mytype");
    tag.setRemoteId("rid");
    auto createjob = new TagCreateJob(tag, this);
    AKVERIFYEXEC(createjob);
    QVERIFY(createjob->tag().isValid());

    {
        auto fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().setFetchRemoteId(true);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QCOMPARE(fetchJob->tags().first().gid(), QByteArray("gid"));
        QCOMPARE(fetchJob->tags().first().type(), QByteArray("mytype"));
        QCOMPARE(fetchJob->tags().first().remoteId(), QByteArray("rid"));

        auto deleteJob = new TagDeleteJob(fetchJob->tags().first(), this);
        AKVERIFYEXEC(deleteJob);
    }
    {
        auto select = new ResourceSelectJob(QStringLiteral(""));
        AKVERIFYEXEC(select);
    }
}

void TagTest::testRIDIsolation()
{
    {
        auto select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);
    }

    Tag tag;
    tag.setGid("gid");
    tag.setType("mytype");
    tag.setRemoteId("rid_0");

    auto createJob = new TagCreateJob(tag, this);
    AKVERIFYEXEC(createJob);
    QVERIFY(createJob->tag().isValid());

    qint64 tagId;
    {
        auto fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().setFetchRemoteId(true);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().count(), 1);
        QCOMPARE(fetchJob->tags().first().gid(), QByteArray("gid"));
        QCOMPARE(fetchJob->tags().first().type(), QByteArray("mytype"));
        QCOMPARE(fetchJob->tags().first().remoteId(), QByteArray("rid_0"));
        tagId = fetchJob->tags().first().id();
    }

    {
        auto select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_1"));
        AKVERIFYEXEC(select);
    }

    tag.setRemoteId("rid_1");
    createJob = new TagCreateJob(tag, this);
    createJob->setMergeIfExisting(true);
    AKVERIFYEXEC(createJob);
    QVERIFY(createJob->tag().isValid());

    {
        auto fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().setFetchRemoteId(true);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().count(), 1);
        QCOMPARE(fetchJob->tags().first().gid(), QByteArray("gid"));
        QCOMPARE(fetchJob->tags().first().type(), QByteArray("mytype"));
        QCOMPARE(fetchJob->tags().first().remoteId(), QByteArray("rid_1"));

        QCOMPARE(fetchJob->tags().first().id(), tagId);
    }

    auto deleteJob = new TagDeleteJob(Tag(tagId), this);
    AKVERIFYEXEC(deleteJob);

    {
        auto select = new ResourceSelectJob(QStringLiteral(""));
        AKVERIFYEXEC(select);
    }
}

void TagTest::testDelete()
{
    Akonadi::Monitor monitor;
    monitor.setTypeMonitored(Monitor::Tags);
    QSignalSpy spy(&monitor, &Monitor::tagRemoved);

    Tag tag1;
    {
        tag1.setGid("tag1");
        auto createjob = new TagCreateJob(tag1, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        tag1 = createjob->tag();
    }
    Tag tag2;
    {
        tag2.setGid("tag2");
        auto createjob = new TagCreateJob(tag2, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        tag2 = createjob->tag();
    }
    {
        auto deleteJob = new TagDeleteJob(tag1, this);
        AKVERIFYEXEC(deleteJob);
    }

    {
        auto fetchJob = new TagFetchJob(this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QCOMPARE(fetchJob->tags().first().gid(), tag2.gid());
    }
    {
        auto deleteJob = new TagDeleteJob(tag2, this);
        AKVERIFYEXEC(deleteJob);
    }

    // Collect Remove notification, so that they don't interfere with testDeleteRIDIsolation
    QTRY_VERIFY(!spy.isEmpty());
}

void TagTest::testDeleteRIDIsolation()
{
    Tag tag;
    tag.setGid("gid");
    tag.setType("mytype");
    tag.setRemoteId("rid_0");

    {
        auto select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        auto createJob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createJob);
        QVERIFY(createJob->tag().isValid());
        tag.setId(createJob->tag().id());
    }

    tag.setRemoteId("rid_1");
    {
        auto select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_1"));
        AKVERIFYEXEC(select);

        auto createJob = new TagCreateJob(tag, this);
        createJob->setMergeIfExisting(true);
        AKVERIFYEXEC(createJob);
        QVERIFY(createJob->tag().isValid());
    }

    auto monitor = AkonadiTest::getTestMonitor();
    QSignalSpy signalSpy(monitor.get(), &Monitor::tagRemoved);

    auto deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);

    // Other tests notifications might interfere due to notification compression on server
    QTRY_VERIFY(signalSpy.count() >= 1);

    Tag removedTag;
    while (!signalSpy.isEmpty()) {
        const Tag t = signalSpy.takeFirst().takeFirst().value<Akonadi::Tag>();
        if (t.id() == tag.id()) {
            removedTag = t;
            break;
        }
    }

    QVERIFY(removedTag.isValid());
    QVERIFY(removedTag.remoteId().isEmpty());

    {
        auto select = new ResourceSelectJob(QStringLiteral(""), this);
        AKVERIFYEXEC(select);
    }
}

void TagTest::testModify()
{
    Tag tag;
    {
        tag.setGid("gid");
        auto createjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        tag = createjob->tag();
    }

    // We can add an attribute
    {
        auto attr = tag.attribute<Akonadi::TagAttribute>(Tag::AddIfMissing);
        attr->setDisplayName(QStringLiteral("display name"));
        tag.setParent(Tag(0));
        tag.setType("mytype");
        auto modJob = new TagModifyJob(tag, this);
        AKVERIFYEXEC(modJob);

        auto fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QVERIFY(fetchJob->tags().first().hasAttribute<Akonadi::TagAttribute>());
    }
    // We can update an attribute
    {
        auto attr = tag.attribute<Akonadi::TagAttribute>(Tag::AddIfMissing);
        attr->setDisplayName(QStringLiteral("display name2"));
        auto modJob = new TagModifyJob(tag, this);
        AKVERIFYEXEC(modJob);

        auto fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QVERIFY(fetchJob->tags().first().hasAttribute<Akonadi::TagAttribute>());
        QCOMPARE(fetchJob->tags().first().attribute<Akonadi::TagAttribute>()->displayName(), attr->displayName());
    }
    // We can clear an attribute
    {
        tag.removeAttribute<Akonadi::TagAttribute>();
        auto modJob = new TagModifyJob(tag, this);
        AKVERIFYEXEC(modJob);

        auto fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QVERIFY(!fetchJob->tags().first().hasAttribute<Akonadi::TagAttribute>());
    }

    auto deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testModifyFromResource()
{
    auto select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
    AKVERIFYEXEC(select);

    Tag tag;
    {
        tag.setGid("gid");
        tag.setRemoteId("rid");
        auto createjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        tag = createjob->tag();
    }

    {
        tag.setRemoteId(QByteArray(""));
        auto modJob = new TagModifyJob(tag, this);
        AKVERIFYEXEC(modJob);

        // The tag is removed on the server, because we just removed the last
        // RemoteID
        auto fetchJob = new TagFetchJob(this);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 0);
    }
}

void TagTest::testCreateMerge()
{
    Tag tag;
    {
        tag.setGid("gid");
        auto createjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        tag = createjob->tag();
    }
    {
        Tag tag2;
        tag2.setGid("gid");
        auto createjob = new TagCreateJob(tag2, this);
        createjob->setMergeIfExisting(true);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        QCOMPARE(createjob->tag().id(), tag.id());
    }

    auto deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testAttributes()
{
    Tag tag;
    {
        tag.setGid("gid2");
        auto attr = tag.attribute<TagAttribute>(Tag::AddIfMissing);
        attr->setDisplayName(QStringLiteral("name"));
        attr->setInToolbar(true);
        auto createjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        tag = createjob->tag();

        {
            auto fetchJob = new TagFetchJob(createjob->tag(), this);
            fetchJob->fetchScope().fetchAttribute<TagAttribute>();
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->tags().size(), 1);
            QVERIFY(fetchJob->tags().first().hasAttribute<TagAttribute>());
            // we need to clone because the returned attribute is just a reference and destroyed on the next line
            // FIXME we should find a better solution for this (like returning a smart pointer or value object)
            QScopedPointer<TagAttribute> tagAttr(fetchJob->tags().first().attribute<TagAttribute>()->clone());
            QVERIFY(tagAttr);
            QCOMPARE(tagAttr->displayName(), QStringLiteral("name"));
            QCOMPARE(tagAttr->inToolbar(), true);
        }
    }
    // Try fetching multiple items
    Tag tag2;
    {
        tag2.setGid("gid22");
        TagAttribute *attr = tag.attribute<TagAttribute>(Tag::AddIfMissing)->clone();
        attr->setDisplayName(QStringLiteral("name2"));
        attr->setInToolbar(true);
        tag2.addAttribute(attr);
        auto createjob = new TagCreateJob(tag2, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        tag2 = createjob->tag();

        {
            auto fetchJob = new TagFetchJob(Tag::List() << tag << tag2, this);
            fetchJob->fetchScope().fetchAttribute<TagAttribute>();
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->tags().size(), 2);
            QVERIFY(fetchJob->tags().at(0).hasAttribute<TagAttribute>());
            QVERIFY(fetchJob->tags().at(1).hasAttribute<TagAttribute>());
        }
    }

    auto deleteJob = new TagDeleteJob(Tag::List() << tag << tag2, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testTagItem()
{
    Akonadi::Monitor monitor;
    monitor.itemFetchScope().setFetchTags(true);
    monitor.setAllMonitored(true);
    QVERIFY(AkonadiTest::akWaitForSignal(&monitor, &Monitor::monitorReady));

    const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    Tag tag;
    {
        auto createjob = new TagCreateJob(Tag(QStringLiteral("gid1")), this);
        AKVERIFYEXEC(createjob);
        tag = createjob->tag();
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }

    item1.setTag(tag);

    QSignalSpy tagsSpy(&monitor, &Monitor::itemsTagsChanged);

    auto modJob = new ItemModifyJob(item1, this);
    AKVERIFYEXEC(modJob);

    QTRY_VERIFY(tagsSpy.count() >= 1);
    QTRY_COMPARE(tagsSpy.last().first().value<Akonadi::Item::List>().first().id(), item1.id());
    QTRY_COMPARE(tagsSpy.last().at(1).value<QSet<Tag>>().size(), 1); // 1 added tag

    auto fetchJob = new ItemFetchJob(item1, this);
    fetchJob->fetchScope().setFetchTags(true);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().first().tags().size(), 1);

    auto deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testCreateItem()
{
    const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    Tag tag;
    {
        auto createjob = new TagCreateJob(Tag(QStringLiteral("gid1")), this);
        AKVERIFYEXEC(createjob);
        tag = createjob->tag();
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        item1.setTag(tag);
        auto append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }

    auto fetchJob = new ItemFetchJob(item1, this);
    fetchJob->fetchScope().setFetchTags(true);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().first().tags().size(), 1);

    auto deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testCreateItemWithTags()
{
    const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    Tag tag1;
    {
        auto createjob = new TagCreateJob(Tag(QStringLiteral("gid1")), this);
        AKVERIFYEXEC(createjob);
        tag1 = createjob->tag();
    }
    Tag tag2;
    {
        auto createjob = new TagCreateJob(Tag(QStringLiteral("gid2")), this);
        AKVERIFYEXEC(createjob);
        tag2 = createjob->tag();
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        item1.setTags({tag1, tag2});
        auto append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }

    auto fetchJob = new ItemFetchJob(item1, this);
    fetchJob->fetchScope().setFetchTags(true);
    AKVERIFYEXEC(fetchJob);
    auto fetchTags = fetchJob->items().first().tags();

    std::unique_ptr<TagDeleteJob, void (*)(TagDeleteJob *)> finally(new TagDeleteJob({tag1, tag2}, this), [](TagDeleteJob *j) {
        j->exec();
        delete j;
    });
    QCOMPARE(fetchTags.size(), 2);
    QVERIFY(fetchTags.contains(tag1));
    QVERIFY(fetchTags.contains(tag2));
}

void TagTest::testFetchTagIdWithItem()
{
    const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    Tag tag;
    {
        auto createjob = new TagCreateJob(Tag(QStringLiteral("gid1")), this);
        AKVERIFYEXEC(createjob);
        tag = createjob->tag();
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        item1.setTag(tag);
        auto append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }

    auto fetchJob = new ItemFetchJob(item1, this);
    fetchJob->fetchScope().setFetchTags(true);
    fetchJob->fetchScope().tagFetchScope().setFetchIdOnly(true);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().first().tags().size(), 1);
    Tag t = fetchJob->items().first().tags().first();
    QCOMPARE(t.id(), tag.id());
    QVERIFY(t.gid().isEmpty());

    auto deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testFetchFullTagWithItem()
{
    const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    Tag tag;
    {
        auto createjob = new TagCreateJob(Tag(QStringLiteral("gid1")), this);
        AKVERIFYEXEC(createjob);
        tag = createjob->tag();
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
        // FIXME This should also be possible with create, but isn't
        item1.setTag(tag);
    }

    auto modJob = new ItemModifyJob(item1, this);
    AKVERIFYEXEC(modJob);

    auto fetchJob = new ItemFetchJob(item1, this);
    fetchJob->fetchScope().setFetchTags(true);
    fetchJob->fetchScope().tagFetchScope().setFetchIdOnly(false);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().first().tags().size(), 1);
    Tag t = fetchJob->items().first().tags().first();
    QCOMPARE(t, tag);
    QVERIFY(!t.gid().isEmpty());

    auto deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testModifyItemWithTagByGID()
{
    const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    {
        Tag tag;
        tag.setGid("gid2");
        auto createjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createjob);
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }

    Tag tag;
    tag.setGid("gid2");
    item1.setTag(tag);

    auto modJob = new ItemModifyJob(item1, this);
    AKVERIFYEXEC(modJob);

    auto fetchJob = new ItemFetchJob(item1, this);
    fetchJob->fetchScope().setFetchTags(true);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().first().tags().size(), 1);

    auto deleteJob = new TagDeleteJob(fetchJob->items().first().tags().first(), this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::testModifyItemWithTagByRID()
{
    {
        auto select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);
    }

    const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    Tag tag3;
    {
        tag3.setGid("gid3");
        tag3.setRemoteId("rid3");
        auto createjob = new TagCreateJob(tag3, this);
        AKVERIFYEXEC(createjob);
        tag3 = createjob->tag();
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
    }

    Tag tag;
    tag.setRemoteId("rid2");
    item1.setTag(tag);

    auto modJob = new ItemModifyJob(item1, this);
    AKVERIFYEXEC(modJob);

    auto fetchJob = new ItemFetchJob(item1, this);
    fetchJob->fetchScope().setFetchTags(true);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().first().tags().size(), 1);

    {
        auto deleteJob = new TagDeleteJob(fetchJob->items().first().tags().first(), this);
        AKVERIFYEXEC(deleteJob);
    }

    {
        auto deleteJob = new TagDeleteJob(tag3, this);
        AKVERIFYEXEC(deleteJob);
    }

    {
        auto select = new ResourceSelectJob(QStringLiteral(""));
        AKVERIFYEXEC(select);
    }
}

void TagTest::testMonitor()
{
    Akonadi::Monitor monitor;
    monitor.setAllMonitored(true);
    monitor.itemFetchScope().setFetchTags(true);
    monitor.tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();
    QVERIFY(AkonadiTest::akWaitForSignal(&monitor, &Monitor::monitorReady));

    Akonadi::Tag createdTag;
    {
        QSignalSpy addedSpy(&monitor, &Monitor::tagAdded);
        QVERIFY(addedSpy.isValid());
        Tag tag;
        tag.setGid("gid2");
        tag.setName(QStringLiteral("name2"));
        tag.setType("type2");
        auto createjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createjob);
        createdTag = createjob->tag();
        QCOMPARE(createdTag.type(), tag.type());
        QCOMPARE(createdTag.name(), tag.name());
        QCOMPARE(createdTag.gid(), tag.gid());
        // We usually pick up signals from the previous tests as well (due to server-side notification caching)
        QTRY_VERIFY(addedSpy.count() >= 1);
        QTRY_COMPARE(addedSpy.last().first().value<Akonadi::Tag>().id(), createdTag.id());
        const auto notifiedTag = addedSpy.last().first().value<Akonadi::Tag>();
        QCOMPARE(notifiedTag.type(), createdTag.type());
        QCOMPARE(notifiedTag.gid(), createdTag.gid());
        QVERIFY(notifiedTag.hasAttribute<Akonadi::TagAttribute>());
        QCOMPARE(notifiedTag.name(), createdTag.name()); // requires the TagAttribute
    }

    {
        QSignalSpy modifiedSpy(&monitor, &Monitor::tagChanged);
        QVERIFY(modifiedSpy.isValid());
        createdTag.setName(QStringLiteral("name3"));

        auto modJob = new TagModifyJob(createdTag, this);
        AKVERIFYEXEC(modJob);
        // We usually pick up signals from the previous tests as well (due to server-side notification caching)
        QTRY_VERIFY(modifiedSpy.count() >= 1);
        QTRY_COMPARE(modifiedSpy.last().first().value<Akonadi::Tag>().id(), createdTag.id());
        const auto notifiedTag = modifiedSpy.last().first().value<Akonadi::Tag>();
        QCOMPARE(notifiedTag.type(), createdTag.type());
        QCOMPARE(notifiedTag.gid(), createdTag.gid());
        QVERIFY(notifiedTag.hasAttribute<Akonadi::TagAttribute>());
        QCOMPARE(notifiedTag.name(), createdTag.name()); // requires the TagAttribute
    }

    {
        QSignalSpy removedSpy(&monitor, &Monitor::tagRemoved);
        QVERIFY(removedSpy.isValid());
        auto deletejob = new TagDeleteJob(createdTag, this);
        AKVERIFYEXEC(deletejob);
        QTRY_VERIFY(removedSpy.count() >= 1);
        QTRY_COMPARE(removedSpy.last().first().value<Akonadi::Tag>().id(), createdTag.id());
        const auto notifiedTag = removedSpy.last().first().value<Akonadi::Tag>();
        QCOMPARE(notifiedTag.type(), createdTag.type());
        QCOMPARE(notifiedTag.gid(), createdTag.gid());
        QVERIFY(notifiedTag.hasAttribute<Akonadi::TagAttribute>());
        QCOMPARE(notifiedTag.name(), createdTag.name()); // requires the TagAttribute
    }

    {
        QSignalSpy itemsTagsChanged(&monitor, &Monitor::itemsTagsChanged);
        // create a tag
        Tag tag(QStringLiteral("name4"));
        auto tagCreateJob = new TagCreateJob(tag);
        AKVERIFYEXEC(tagCreateJob);
        tag = tagCreateJob->tag();
        // create item
        const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
        Item item;
        item.setParentCollection(res3);
        item.setMimeType(QStringLiteral("application/octet-stream"));
        auto itemCreateJob = new ItemCreateJob(item, res3);
        AKVERIFYEXEC(itemCreateJob);
        item = itemCreateJob->item();

        // add tag to item
        item.setTag(tag);
        auto itemModifyJob = new ItemModifyJob(item);
        AKVERIFYEXEC(itemModifyJob);

        QTRY_VERIFY(itemsTagsChanged.count() >= 1);
        QTRY_COMPARE(itemsTagsChanged.last().first().value<Akonadi::Item::List>().size(), 1);
        qDebug() << itemsTagsChanged.last().first().value<Akonadi::Item::List>().first().tags();
        QTRY_COMPARE(itemsTagsChanged.last().at(1).value<QSet<Tag>>().size(), 1); // 1 added tag
        QTRY_COMPARE(itemsTagsChanged.last().at(2).value<QSet<Tag>>().size(), 0); // no tags removed

        const auto notifiedItem = itemsTagsChanged.last().first().value<Akonadi::Item::List>().first();
        const auto notifiedTag = *itemsTagsChanged.last().at(1).value<QSet<Tag>>().begin();
        QCOMPARE(notifiedItem.id(), item.id());
        QCOMPARE(notifiedItem.tags(), QList{notifiedTag});
        QCOMPARE(notifiedTag, tag);

        // Cleanup
        auto tagDeleteJob = new TagDeleteJob(tag, this);
        AKVERIFYEXEC(tagDeleteJob);
        auto itemDeleteJob = new ItemDeleteJob(item);
        AKVERIFYEXEC(itemDeleteJob);
    }
}

void TagTest::testTagAttributeConfusionBug()
{
    // Create two tags
    Tag firstTag;
    {
        firstTag.setGid("gid");
        firstTag.setName(QStringLiteral("display name"));
        auto createjob = new TagCreateJob(firstTag, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        firstTag = createjob->tag();
    }
    Tag secondTag;
    {
        secondTag.setGid("AnotherGID");
        secondTag.setName(QStringLiteral("another name"));
        auto createjob = new TagCreateJob(secondTag, this);
        AKVERIFYEXEC(createjob);
        QVERIFY(createjob->tag().isValid());
        secondTag = createjob->tag();
    }

    Akonadi::Monitor monitor;
    monitor.setTypeMonitored(Akonadi::Monitor::Tags);
    QVERIFY(AkonadiTest::akWaitForSignal(&monitor, &Monitor::monitorReady));

    const QList<Tag::Id> firstTagIdList{firstTag.id()};

    // Modify attribute on the first tag
    // and check the notification
    {
        QSignalSpy modifiedSpy(&monitor, &Akonadi::Monitor::tagChanged);

        firstTag.setName(QStringLiteral("renamed"));
        auto modJob = new TagModifyJob(firstTag, this);
        AKVERIFYEXEC(modJob);

        auto fetchJob = new TagFetchJob(firstTagIdList, this);
        QVERIFY(fetchJob->fetchScope().fetchAllAttributes());
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        QCOMPARE(fetchJob->tags().first().name(), firstTag.name());

        QTRY_VERIFY(modifiedSpy.count() >= 1);
        QTRY_COMPARE(modifiedSpy.last().first().value<Akonadi::Tag>().id(), firstTag.id());
        const auto notifiedTag = modifiedSpy.last().first().value<Akonadi::Tag>();
        QCOMPARE(notifiedTag.name(), firstTag.name());
    }

    // Cleanup
    auto deleteJob = new TagDeleteJob(firstTag, this);
    AKVERIFYEXEC(deleteJob);
    auto anotherDeleteJob = new TagDeleteJob(secondTag, this);
    AKVERIFYEXEC(anotherDeleteJob);
}

void TagTest::testFetchItemsByTag()
{
    const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
    Tag tag;
    {
        auto createjob = new TagCreateJob(Tag(QStringLiteral("gid1")), this);
        AKVERIFYEXEC(createjob);
        tag = createjob->tag();
    }

    Item item1;
    {
        item1.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item1, res3, this);
        AKVERIFYEXEC(append);
        item1 = append->item();
        // FIXME This should also be possible with create, but isn't
        item1.setTag(tag);
    }

    auto modJob = new ItemModifyJob(item1, this);
    AKVERIFYEXEC(modJob);

    auto fetchJob = new ItemFetchJob(tag, this);
    AKVERIFYEXEC(fetchJob);
    QCOMPARE(fetchJob->items().size(), 1);
    Item i = fetchJob->items().first();
    QCOMPARE(i, item1);

    auto deleteJob = new TagDeleteJob(tag, this);
    AKVERIFYEXEC(deleteJob);
}

void TagTest::tagModifyJobShouldOnlySendModifiedAttributes()
{
    // Given a tag with an attribute
    Tag tag(QStringLiteral("tagWithAttr"));
    auto attr = new Akonadi::TagAttribute;
    attr->setDisplayName(QStringLiteral("display name"));
    tag.addAttribute(attr);
    {
        auto createjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createjob);
        tag = createjob->tag();
    }

    // When one job modifies this attribute, and another one does an unrelated modify job
    Tag attrModTag(tag.id());
    auto modAttr = attrModTag.attribute<Akonadi::TagAttribute>(Tag::AddIfMissing);
    modAttr->setDisplayName(QStringLiteral("modified"));
    auto attrModJob = new TagModifyJob(attrModTag, this);
    AKVERIFYEXEC(attrModJob);

    tag.setType(Tag::GENERIC);
    // this job shouldn't send the old attribute again
    auto modJob = new TagModifyJob(tag, this);
    AKVERIFYEXEC(modJob);

    // Then the tag should have both the modified attribute and the modified type
    {
        auto fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        const Tag fetchedTag = fetchJob->tags().at(0);
        QVERIFY(fetchedTag.hasAttribute<Akonadi::TagAttribute>());
        QCOMPARE(fetchedTag.attribute<Akonadi::TagAttribute>()->displayName(), QStringLiteral("modified"));
        QCOMPARE(fetchedTag.type(), Tag::GENERIC);
    }

    // And when adding a new attribute next to the old one
    auto attr2 = AttributeFactory::createAttribute("SecondType");
    tag.addAttribute(attr2);
    // this job shouldn't send the old attribute again
    auto modJob2 = new TagModifyJob(tag, this);
    AKVERIFYEXEC(modJob2);

    // Then the tag should have the modified attribute and the second one
    {
        auto fetchJob = new TagFetchJob(this);
        fetchJob->fetchScope().setFetchAllAttributes(true);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->tags().size(), 1);
        const Tag fetchedTag = fetchJob->tags().at(0);
        QVERIFY(fetchedTag.hasAttribute<Akonadi::TagAttribute>());
        QCOMPARE(fetchedTag.attribute<Akonadi::TagAttribute>()->displayName(), QStringLiteral("modified"));
        QCOMPARE(fetchedTag.type(), Tag::GENERIC);
        QVERIFY(fetchedTag.attribute("SecondType"));
    }
}

void TagTest::testItemNotificationOnTagDeletion()
{
    Akonadi::Monitor monitor;
    monitor.setAllMonitored(true);
    monitor.itemFetchScope().setFetchTags(true);
    monitor.tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();
    QVERIFY(AkonadiTest::akWaitForSignal(&monitor, &Monitor::monitorReady));

    // Create item
    Item item;
    {
        const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
        item.setMimeType(QStringLiteral("application/octet-stream"));
        auto append = new ItemCreateJob(item, res3, this);
        AKVERIFYEXEC(append);
        item = append->item();
    }

    // Create tag
    Tag tag(QStringLiteral("tag"));
    {
        auto createjob = new TagCreateJob(tag, this);
        AKVERIFYEXEC(createjob);
        tag = createjob->tag();
    }

    // Add tag to item
    {
        item.setTag(tag);
        auto modJob = new ItemModifyJob(item, this);
        AKVERIFYEXEC(modJob);
    }

    QSignalSpy itemsTagsChanged(&monitor, &Monitor::itemsTagsChanged);

    // Delete the tag
    {
        auto deleteJob = new TagDeleteJob(tag, this);
        AKVERIFYEXEC(deleteJob);
    }

    QTRY_VERIFY(itemsTagsChanged.count() >= 1);
    const auto notifiedItem = itemsTagsChanged.last().first().value<Akonadi::Item::List>().first();
    QCOMPARE(notifiedItem.id(), item.id());
    const auto addedTags = itemsTagsChanged.last().at(1).value<QSet<Tag>>();
    QVERIFY(addedTags.isEmpty());
    const auto removedTags = itemsTagsChanged.last().at(2).value<QSet<Tag>>();
    QCOMPARE(removedTags.size(), 1);
    QCOMPARE(*removedTags.begin(), tag);
}

#include "tagtest.moc"

QTEST_AKONADI_CORE_MAIN(TagTest)
