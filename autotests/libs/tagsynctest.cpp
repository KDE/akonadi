/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagsync.h"
#include "agentinstance.h"
#include "agentmanager.h"
#include "control.h"
#include "itemcreatejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "resourceselectjob_p.h"
#include "tag.h"
#include "tagcreatejob.h"
#include "tagdeletejob.h"
#include "tagfetchjob.h"
#include "tagfetchscope.h"

#include "qtest_akonadi.h"

#include <QObject>

using namespace Akonadi;

class TagSyncTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
        Control::start();
        AkonadiTest::setAllResourcesOffline();
        cleanTags();
    }

    Tag::List getTags()
    {
        auto fetchJob = new TagFetchJob();
        fetchJob->fetchScope().setFetchRemoteId(true);
        bool ret = fetchJob->exec();
        Q_ASSERT(ret);
        return fetchJob->tags();
    }

    Tag::List getTagsWithRid()
    {
        Tag::List tags;
        Q_FOREACH (const Tag &t, getTags()) {
            if (!t.remoteId().isEmpty()) {
                tags << t;
                qDebug() << t.remoteId();
            }
        }
        return tags;
    }

    void cleanTags()
    {
        Q_FOREACH (const Tag &t, getTags()) {
            auto job = new TagDeleteJob(t);
            bool ret = job->exec();
            Q_ASSERT(ret);
        }
    }

    void newTag()
    {
        ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag::List remoteTags;

        Tag tag1(QStringLiteral("tag1"));
        tag1.setRemoteId("rid1");
        remoteTags << tag1;

        auto syncer = new TagSync(this);
        syncer->setFullTagList(remoteTags);
        syncer->setTagMembers(QHash<QString, Item::List>());
        AKVERIFYEXEC(syncer);

        Tag::List resultTags = getTags();
        QCOMPARE(resultTags.count(), remoteTags.count());
        QCOMPARE(resultTags, remoteTags);
        cleanTags();
    }

    void newTagWithItems()
    {
        const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));
        ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_2"));
        AKVERIFYEXEC(select);

        Tag::List remoteTags;

        Tag tag1(QStringLiteral("tag1"));
        tag1.setRemoteId("rid1");
        remoteTags << tag1;

        Item item1;
        {
            item1.setMimeType(QStringLiteral("application/octet-stream"));
            item1.setRemoteId(QStringLiteral("item1"));
            auto append = new ItemCreateJob(item1, res3, this);
            AKVERIFYEXEC(append);
            item1 = append->item();
        }

        QHash<QString, Item::List> tagMembers;
        tagMembers.insert(QString::fromLatin1(tag1.remoteId()), {item1});

        auto syncer = new TagSync(this);
        syncer->setFullTagList(remoteTags);
        syncer->setTagMembers(tagMembers);
        AKVERIFYEXEC(syncer);

        Tag::List resultTags = getTags();
        QCOMPARE(resultTags.count(), remoteTags.count());
        QCOMPARE(resultTags, remoteTags);

        // We need the id of the fetch
        tag1 = resultTags.first();

        auto fetchJob = new ItemFetchJob(tag1);
        fetchJob->fetchScope().setFetchTags(true);
        fetchJob->fetchScope().tagFetchScope().setFetchRemoteId(true);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(fetchJob->items().count(), tagMembers.value(QString::fromLatin1(tag1.remoteId())).count());
        QCOMPARE(fetchJob->items(), tagMembers.value(QString::fromLatin1(tag1.remoteId())));

        cleanTags();
    }

    void existingTag()
    {
        ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag tag1(QStringLiteral("tag1"));
        tag1.setRemoteId("rid1");

        auto createJob = new TagCreateJob(tag1, this);
        AKVERIFYEXEC(createJob);

        Tag::List remoteTags;
        remoteTags << tag1;

        auto syncer = new TagSync(this);
        syncer->setFullTagList(remoteTags);
        syncer->setTagMembers(QHash<QString, Item::List>());
        AKVERIFYEXEC(syncer);

        Tag::List resultTags = getTags();
        QCOMPARE(resultTags.count(), remoteTags.count());
        QCOMPARE(resultTags, remoteTags);
        cleanTags();
    }

    void existingTagWithItems()
    {
        const Collection res3 = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res3")));

        ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_2"));
        AKVERIFYEXEC(select);

        Tag tag1(QStringLiteral("tag1"));
        tag1.setRemoteId("rid1");

        auto createJob = new TagCreateJob(tag1, this);
        AKVERIFYEXEC(createJob);

        Tag::List remoteTags;
        remoteTags << tag1;

        Item item1;
        {
            item1.setMimeType(QStringLiteral("application/octet-stream"));
            item1.setRemoteId(QStringLiteral("item1"));
            auto append = new ItemCreateJob(item1, res3, this);
            AKVERIFYEXEC(append);
            item1 = append->item();
        }

        Item item2;
        {
            item2.setMimeType(QStringLiteral("application/octet-stream"));
            item2.setRemoteId(QStringLiteral("item2"));
            item2.setTag(tag1);
            auto append = new ItemCreateJob(item2, res3, this);
            AKVERIFYEXEC(append);
            item2 = append->item();
        }

        QHash<QString, Item::List> tagMembers;
        tagMembers.insert(QString::fromLatin1(tag1.remoteId()), Item::List() << item1);

        auto syncer = new TagSync(this);
        syncer->setFullTagList(remoteTags);
        syncer->setTagMembers(tagMembers);
        AKVERIFYEXEC(syncer);

        Tag::List resultTags = getTags();
        QCOMPARE(resultTags.count(), remoteTags.count());
        QCOMPARE(resultTags, remoteTags);
        {
            auto fetchJob = new ItemFetchJob(item1, this);
            fetchJob->fetchScope().setFetchTags(true);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().first().tags().count(), 1);
        }
        {
            auto fetchJob = new ItemFetchJob(item2, this);
            fetchJob->fetchScope().setFetchTags(true);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().first().tags().count(), 0);
        }

        cleanTags();
    }

    void removeTag()
    {
        ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag tag1(QStringLiteral("tag1"));
        tag1.setRemoteId("rid1");

        auto createJob = new TagCreateJob(tag1, this);
        AKVERIFYEXEC(createJob);

        Tag::List remoteTags;

        auto syncer = new TagSync(this);
        syncer->setFullTagList(remoteTags);
        syncer->setTagMembers(QHash<QString, Item::List>());
        AKVERIFYEXEC(syncer);

        Tag::List resultTags = getTagsWithRid();
        QCOMPARE(resultTags.count(), remoteTags.count());
        QCOMPARE(resultTags, remoteTags);
        cleanTags();
    }
};

QTEST_AKONADIMAIN(TagSyncTest)

#include "tagsynctest.moc"
