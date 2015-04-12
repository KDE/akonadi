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
namespace Akonadi {
    class Item;
}

#include "tagsync.h"

#include "jobs/itemfetchjob.h"
#include "itemfetchscope.h"
#include "jobs/itemmodifyjob.h"
#include "jobs/tagfetchjob.h"
#include "jobs/tagcreatejob.h"
#include "jobs/tagmodifyjob.h"

using namespace Akonadi;

bool operator==(const Item &left, const Item &right)
{
    if (left.isValid() && right.isValid() && (left.id() == right.id())) {
        return true;
    }
    if (!left.remoteId().isEmpty() && !right.remoteId().isEmpty() && (left.remoteId() == right.remoteId())) {
        return true;
    }
    if (!left.gid().isEmpty() && !right.gid().isEmpty() && (left.gid() == right.gid())) {
        return true;
    }
    return false;
}

TagSync::TagSync(QObject *parent)
    : Job(parent),
    mDeliveryDone(false),
    mTagMembersDeliveryDone(false),
    mLocalTagsFetched(false)
{

}

TagSync::~TagSync()
{

}

void TagSync::setFullTagList(const Akonadi::Tag::List &tags)
{
    mRemoteTags = tags;
    mDeliveryDone = true;
    diffTags();
}

void TagSync::setTagMembers(const QHash<QString, Akonadi::Item::List> &ridMemberMap)
{
    mRidMemberMap = ridMemberMap;
    mTagMembersDeliveryDone = true;
    diffTags();
}

void TagSync::doStart()
{
    // qDebug();
    //This should include all tags, including the ones that don't have a remote id
    Akonadi::TagFetchJob *fetch = new Akonadi::TagFetchJob(this);
    connect(fetch, SIGNAL(result(KJob*)), this, SLOT(onLocalTagFetchDone(KJob*)));
}

void TagSync::onLocalTagFetchDone(KJob *job)
{
    // qDebug();
    TagFetchJob *fetch = static_cast<TagFetchJob*>(job);
    mLocalTags = fetch->tags();
    mLocalTagsFetched = true;
    diffTags();
}

void TagSync::diffTags()
{
    if (!mDeliveryDone || !mTagMembersDeliveryDone || !mLocalTagsFetched) {
        qDebug() << "waiting for delivery: " << mDeliveryDone << mLocalTagsFetched;
        return;
    }
    // qDebug() << "diffing";
    QHash<QByteArray, Akonadi::Tag> tagByGid;
    QHash<QByteArray, Akonadi::Tag> tagByRid;
    QHash<Akonadi::Tag::Id, Akonadi::Tag> tagById;
    Q_FOREACH (const Akonadi::Tag &localTag, mLocalTags) {
        tagByRid.insert(localTag.remoteId(), localTag);
        tagByGid.insert(localTag.gid(), localTag);
        if (!localTag.remoteId().isEmpty()) {
            tagById.insert(localTag.id(), localTag);
        }
    }
    Q_FOREACH (const Akonadi::Tag &remoteTag, mRemoteTags) {
        if (tagByRid.contains(remoteTag.remoteId())) {
            //Tag still exists, check members
            Tag tag = tagByRid.value(remoteTag.remoteId());
            ItemFetchJob *itemFetch = new ItemFetchJob(tag, this);
            itemFetch->setProperty("tag", QVariant::fromValue(tag));
            itemFetch->setProperty("merge", false);
            itemFetch->fetchScope().setFetchGid(true);
            connect(itemFetch, SIGNAL(result(KJob*)), this, SLOT(onTagItemsFetchDone(KJob*)));
            connect(itemFetch, SIGNAL(result(KJob*)), this, SLOT(onJobDone(KJob*)));
            tagById.remove(tagByRid.value(remoteTag.remoteId()).id());
        } else if (tagByGid.contains(remoteTag.gid())) {
            //Tag exists but has no rid
            //Merge members and set rid
            Tag tag = tagByGid.value(remoteTag.gid());
            tag.setRemoteId(remoteTag.remoteId());
            ItemFetchJob *itemFetch = new ItemFetchJob(tag, this);
            itemFetch->setProperty("tag", QVariant::fromValue(tag));
            itemFetch->setProperty("merge", true);
            itemFetch->fetchScope().setFetchGid(true);
            connect(itemFetch, SIGNAL(result(KJob*)), this, SLOT(onTagItemsFetchDone(KJob*)));
            connect(itemFetch, SIGNAL(result(KJob*)), this, SLOT(onJobDone(KJob*)));
            tagById.remove(tagByGid.value(remoteTag.gid()).id());
        } else {
            //New tag, create
            TagCreateJob *createJob = new TagCreateJob(remoteTag, this);
            createJob->setMergeIfExisting(true);
            connect(createJob, SIGNAL(result(KJob*)), this, SLOT(onCreateTagDone(KJob*)));
            connect(createJob, SIGNAL(result(KJob*)), this, SLOT(onJobDone(KJob*)));
        }
    }
    Q_FOREACH (const Akonadi::Tag::Id &removedTag, tagById.keys()) {
        //Removed remotely, unset rid
        Tag tag = tagById.value(removedTag);
        tag.setRemoteId(QByteArray(""));
        TagModifyJob *modJob = new TagModifyJob(tag, this);
        connect(modJob, SIGNAL(result(KJob*)), this, SLOT(onJobDone(KJob*)));
    }
    checkDone();
}

void TagSync::onCreateTagDone(KJob *job)
{
    if (job->error()) {
        qWarning() << "ItemFetch failed: " << job->errorString();
        return;
    }

    Akonadi::Tag tag = static_cast<Akonadi::TagCreateJob*>(job)->tag();
    const Item::List remoteMembers = mRidMemberMap.value(QString::fromLatin1(tag.remoteId()));
    Q_FOREACH (Item item, remoteMembers) {
        item.setTag(tag);
        ItemModifyJob *modJob = new ItemModifyJob(item, this);
        connect(modJob, SIGNAL(result(KJob*)), this, SLOT(onJobDone(KJob*)));
        qDebug() << "setting tag " << item.remoteId();
    }
}

static bool containsByGidOrRid(const Item::List &items, const Item &key)
{
    Q_FOREACH(const Item &item, items) {
        if ((!item.gid().isEmpty() && !key.gid().isEmpty()) && (item.gid() == key.gid())) {
            return true;
        } else if (item.remoteId() == key.remoteId()) {
            return true;
        }
    }
    return false;
}

void TagSync::onTagItemsFetchDone(KJob *job)
{
    if (job->error()) {
        qWarning() << "ItemFetch failed: " << job->errorString();
        return;
    }

    const Akonadi::Item::List items = static_cast<Akonadi::ItemFetchJob*>(job)->items();
    const Akonadi::Tag tag = job->property("tag").value<Akonadi::Tag>();
    const bool merge = job->property("merge").toBool();
    const Item::List remoteMembers = mRidMemberMap.value(QString::fromLatin1(tag.remoteId()));

    //add = remote - local
    Item::List toAdd;
    Q_FOREACH(const Item &remote, remoteMembers) {
        if (!containsByGidOrRid(items, remote)) {
            toAdd << remote;
        }
    }

    //remove = local - remote
    Item::List toRemove;
    Q_FOREACH(const Item &local, items) {
        //Skip items that have no remote id yet
        //Trying to them will only result in a conflict
        if (local.remoteId().isEmpty()) {
            continue;
        }
        if (!containsByGidOrRid(remoteMembers, local)) {
            toRemove << local;
        }
    }

    if (!merge) {
        Q_FOREACH (Item item, toRemove) {
            item.clearTag(tag);
            ItemModifyJob *modJob = new ItemModifyJob(item, this);
            connect(modJob, SIGNAL(result(KJob*)), this, SLOT(onJobDone(KJob*)));
            qDebug() << "removing tag " << item.remoteId();
        }
    }
    Q_FOREACH (Item item, toAdd) {
        item.setTag(tag);
        ItemModifyJob *modJob = new ItemModifyJob(item, this);
        connect(modJob, SIGNAL(result(KJob*)), this, SLOT(onJobDone(KJob*)));
        qDebug() << "setting tag " << item.remoteId();
    }
}

void TagSync::onJobDone(KJob *)
{
    checkDone();
}

void TagSync::slotResult(KJob *job)
{
    if (job->error()) {
        qWarning() << "Error during TagSync: " << job->errorString() << job->metaObject()->className();
        // pretent there were no errors
        Akonadi::Job::removeSubjob(job);
    } else {
        Akonadi::Job::slotResult(job);
    }
}

void TagSync::checkDone()
{
    if (hasSubjobs()) {
        return;
    }
    qDebug() << "done";
    emitResult();
}

#include "tagsync.moc"
