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

#include "test_utils.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstance.h>
#include <akonadi/control.h>
#include <akonadi/tagfetchjob.h>
#include <akonadi/tagdeletejob.h>
#include <akonadi/tagcreatejob.h>
#include <akonadi/tag.h>
#include <akonadi/tagsync.h>
#include <akonadi/resourceselectjob_p.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <QtCore/QObject>
#include <QSignalSpy>

#include <qtest_akonadi.h>

using namespace Akonadi;

bool operator==(const Tag &left, const Tag &right)
{
    if (left.gid() == right.gid()) {
        return true;
    }
    qDebug() << left.gid();
    qDebug() << right.gid();
    return false;
}

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
        TagFetchJob *fetchJob = new TagFetchJob();
        bool ret = fetchJob->exec();
        Q_ASSERT(ret);
        return fetchJob->tags();
    }

    Tag::List getTagsWithRid()
    {
        Tag::List tags;
        Q_FOREACH(const Tag &t, getTags()) {
            if (!t.remoteId().isEmpty()) {
                tags << t;
                kDebug() << t.remoteId();
            }
        }
        return tags;
    }

    void cleanTags()
    {
        Q_FOREACH(const Tag &t, getTags()) {
            TagDeleteJob *job = new TagDeleteJob(t);
            bool ret = job->exec();
            Q_ASSERT(ret);
        }
    }

    void newTag()
    {
        ResourceSelectJob *select = new ResourceSelectJob(QLatin1String("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag::List remoteTags;

        Tag tag1("tag1");
        tag1.setRemoteId("rid1");
        remoteTags << tag1;

        TagSync* syncer = new TagSync(this);
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
        const Collection res3 = Collection( collectionIdFromPath( "res3" ) );
        ResourceSelectJob *select = new ResourceSelectJob(QLatin1String("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag::List remoteTags;

        Tag tag1("tag1");
        tag1.setRemoteId("rid1");
        remoteTags << tag1;

        Item item1;
        {
            item1.setMimeType("application/octet-stream");
            item1.setRemoteId("item1");
            ItemCreateJob *append = new ItemCreateJob(item1, res3, this);
            AKVERIFYEXEC(append);
            item1 = append->item();
        }

        QHash<QString, Item::List> tagMembers;
        tagMembers.insert(tag1.remoteId(), Item::List() << item1);

        TagSync* syncer = new TagSync(this);
        syncer->setFullTagList(remoteTags);
        syncer->setTagMembers(tagMembers);
        AKVERIFYEXEC(syncer);

        Tag::List resultTags = getTags();
        QCOMPARE(resultTags.count(), remoteTags.count());
        QCOMPARE(resultTags, remoteTags);

        //We need the id of the fetch
        tag1 = resultTags.first();

        ItemFetchJob *fetchJob = new ItemFetchJob(tag1);
        AKVERIFYEXEC(fetchJob);
        QCOMPARE(tagMembers.value(tag1.remoteId()).count(), fetchJob->items().count());
        QCOMPARE(tagMembers.value(tag1.remoteId()), fetchJob->items());

        cleanTags();
    }

    void existingTag()
    {
        ResourceSelectJob *select = new ResourceSelectJob(QLatin1String("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag tag1("tag1");
        tag1.setRemoteId("rid1");

        TagCreateJob *createJob = new TagCreateJob(tag1, this);
        AKVERIFYEXEC(createJob);

        Tag::List remoteTags;
        remoteTags << tag1;

        TagSync* syncer = new TagSync(this);
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
        const Collection res3 = Collection( collectionIdFromPath( "res3" ) );

        ResourceSelectJob *select = new ResourceSelectJob(QLatin1String("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag tag1("tag1");
        tag1.setRemoteId("rid1");

        TagCreateJob *createJob = new TagCreateJob(tag1, this);
        AKVERIFYEXEC(createJob);

        Tag::List remoteTags;
        remoteTags << tag1;

        Item item1;
        {
            item1.setMimeType("application/octet-stream");
            item1.setRemoteId("item1");
            ItemCreateJob *append = new ItemCreateJob(item1, res3, this);
            AKVERIFYEXEC(append);
            item1 = append->item();
        }

        Item item2;
        {
            item2.setMimeType("application/octet-stream");
            item2.setRemoteId("item2");
            item2.setTag(tag1);
            ItemCreateJob *append = new ItemCreateJob(item2, res3, this);
            AKVERIFYEXEC(append);
            item2 = append->item();
        }

        QHash<QString, Item::List> tagMembers;
        tagMembers.insert(tag1.remoteId(), Item::List() << item1);

        TagSync* syncer = new TagSync(this);
        syncer->setFullTagList(remoteTags);
        syncer->setTagMembers(tagMembers);
        AKVERIFYEXEC(syncer);

        Tag::List resultTags = getTags();
        QCOMPARE(resultTags.count(), remoteTags.count());
        QCOMPARE(resultTags, remoteTags);
        {
            ItemFetchJob *fetchJob = new ItemFetchJob(item1, this);
            fetchJob->fetchScope().setFetchTags(true);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().first().tags().count(), 1);
        }
        {
            ItemFetchJob *fetchJob = new ItemFetchJob(item2, this);
            fetchJob->fetchScope().setFetchTags(true);
            AKVERIFYEXEC(fetchJob);
            QCOMPARE(fetchJob->items().first().tags().count(), 0);
        }

        cleanTags();
    }

    void removeTag()
    {
        ResourceSelectJob *select = new ResourceSelectJob(QLatin1String("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag tag1("tag1");
        tag1.setRemoteId("rid1");

        TagCreateJob *createJob = new TagCreateJob(tag1, this);
        AKVERIFYEXEC(createJob);

        Tag::List remoteTags;

        TagSync* syncer = new TagSync(this);
        syncer->setFullTagList(remoteTags);
        syncer->setTagMembers(QHash<QString, Item::List>());
        AKVERIFYEXEC(syncer);

        Tag::List resultTags = getTagsWithRid();
        QCOMPARE(resultTags.count(), remoteTags.count());
        QCOMPARE(resultTags, remoteTags);
        cleanTags();
    }
};

QTEST_AKONADIMAIN(TagSyncTest, NoGUI)

#include "tagsynctest.moc"
