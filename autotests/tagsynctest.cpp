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

#include <agentmanager.h>
#include <agentinstance.h>
#include <control.h>
#include <tagfetchjob.h>
#include <tagdeletejob.h>
#include <tagcreatejob.h>
#include <tag.h>
#include <tagsync.h>
#include <resourceselectjob_p.h>
#include <itemcreatejob.h>
#include <itemfetchjob.h>
#include <itemfetchscope.h>

#include <QtCore/QObject>
#include <QSignalSpy>

#include <qtest_akonadi.h>

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
                qDebug() << t.remoteId();
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

        Tag tag1(QStringLiteral("tag1"));
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
        const Collection res3 = Collection( collectionIdFromPath( QStringLiteral("res3") ) );
        ResourceSelectJob *select = new ResourceSelectJob(QLatin1String("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag::List remoteTags;

        Tag tag1(QStringLiteral("tag1"));
        tag1.setRemoteId("rid1");
        remoteTags << tag1;

        Item item1;
        {
            item1.setMimeType(QStringLiteral("application/octet-stream"));
            item1.setRemoteId(QStringLiteral("item1"));
            ItemCreateJob *append = new ItemCreateJob(item1, res3, this);
            AKVERIFYEXEC(append);
            item1 = append->item();
        }

        QHash<QString, Item::List> tagMembers;
        tagMembers.insert(QString::fromLatin1(tag1.remoteId()), Item::List() << item1);

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
        QCOMPARE(tagMembers.value(QString::fromLatin1(tag1.remoteId())).count(), fetchJob->items().count());
        QCOMPARE(tagMembers.value(QString::fromLatin1(tag1.remoteId())), fetchJob->items());

        cleanTags();
    }

    void existingTag()
    {
        ResourceSelectJob *select = new ResourceSelectJob(QLatin1String("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag tag1(QStringLiteral("tag1"));
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
        const Collection res3 = Collection( collectionIdFromPath( QStringLiteral("res3") ) );

        ResourceSelectJob *select = new ResourceSelectJob(QLatin1String("akonadi_knut_resource_0"));
        AKVERIFYEXEC(select);

        Tag tag1(QStringLiteral("tag1"));
        tag1.setRemoteId("rid1");

        TagCreateJob *createJob = new TagCreateJob(tag1, this);
        AKVERIFYEXEC(createJob);

        Tag::List remoteTags;
        remoteTags << tag1;

        Item item1;
        {
            item1.setMimeType(QStringLiteral("application/octet-stream"));
            item1.setRemoteId(QStringLiteral("item1"));
            ItemCreateJob *append = new ItemCreateJob(item1, res3, this);
            AKVERIFYEXEC(append);
            item1 = append->item();
        }

        Item item2;
        {
            item2.setMimeType(QStringLiteral("application/octet-stream"));
            item2.setRemoteId(QStringLiteral("item2"));
            item2.setTag(tag1);
            ItemCreateJob *append = new ItemCreateJob(item2, res3, this);
            AKVERIFYEXEC(append);
            item2 = append->item();
        }

        QHash<QString, Item::List> tagMembers;
        tagMembers.insert(QString::fromLatin1(tag1.remoteId()), Item::List() << item1);

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

        Tag tag1(QStringLiteral("tag1"));
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

QTEST_AKONADIMAIN(TagSyncTest)

#include "tagsynctest.moc"
