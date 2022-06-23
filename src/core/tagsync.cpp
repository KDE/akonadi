/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
namespace Akonadi
{
class Item;
}

#include "tagsync.h"
#include "akonadicore_debug.h"
#include "itemfetchscope.h"
#include "jobs/itemfetchjob.h"
#include "jobs/itemmodifyjob.h"
#include "jobs/tagcreatejob.h"
#include "jobs/tagfetchjob.h"
#include "jobs/tagmodifyjob.h"
#include "tagfetchscope.h"

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
    : Job(parent)
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
    // qCDebug(AKONADICORE_LOG);
    // This should include all tags, including the ones that don't have a remote id
    auto fetch = new Akonadi::TagFetchJob(this);
    fetch->fetchScope().setFetchRemoteId(true);
    connect(fetch, &KJob::result, this, &TagSync::onLocalTagFetchDone);
}

void TagSync::onLocalTagFetchDone(KJob *job)
{
    // qCDebug(AKONADICORE_LOG);
    auto fetch = static_cast<TagFetchJob *>(job);
    mLocalTags = fetch->tags();
    mLocalTagsFetched = true;
    diffTags();
}

void TagSync::diffTags()
{
    if (!mDeliveryDone || !mTagMembersDeliveryDone || !mLocalTagsFetched) {
        qCDebug(AKONADICORE_LOG) << "waiting for delivery: " << mDeliveryDone << mLocalTagsFetched;
        return;
    }
    // qCDebug(AKONADICORE_LOG) << "diffing";
    QHash<QByteArray, Akonadi::Tag> tagByGid;
    QHash<QByteArray, Akonadi::Tag> tagByRid;
    QHash<Akonadi::Tag::Id, Akonadi::Tag> tagById;
    for (const Akonadi::Tag &localTag : std::as_const(mLocalTags)) {
        tagByRid.insert(localTag.remoteId(), localTag);
        tagByGid.insert(localTag.gid(), localTag);
        if (!localTag.remoteId().isEmpty()) {
            tagById.insert(localTag.id(), localTag);
        }
    }
    for (const Akonadi::Tag &remoteTag : std::as_const(mRemoteTags)) {
        if (tagByRid.contains(remoteTag.remoteId())) {
            // Tag still exists, check members
            Tag tag = tagByRid.value(remoteTag.remoteId());
            auto itemFetch = new ItemFetchJob(tag, this);
            itemFetch->setProperty("tag", QVariant::fromValue(tag));
            itemFetch->setProperty("merge", false);
            itemFetch->fetchScope().setFetchGid(true);
            connect(itemFetch, &KJob::result, this, &TagSync::onTagItemsFetchDone);
            connect(itemFetch, &KJob::result, this, &TagSync::onJobDone);
            tagById.remove(tagByRid.value(remoteTag.remoteId()).id());
        } else if (tagByGid.contains(remoteTag.gid())) {
            // Tag exists but has no rid
            // Merge members and set rid
            Tag tag = tagByGid.value(remoteTag.gid());
            tag.setRemoteId(remoteTag.remoteId());
            auto itemFetch = new ItemFetchJob(tag, this);
            itemFetch->setProperty("tag", QVariant::fromValue(tag));
            itemFetch->setProperty("merge", true);
            itemFetch->fetchScope().setFetchGid(true);
            connect(itemFetch, &KJob::result, this, &TagSync::onTagItemsFetchDone);
            connect(itemFetch, &KJob::result, this, &TagSync::onJobDone);
            tagById.remove(tagByGid.value(remoteTag.gid()).id());
        } else {
            // New tag, create
            auto createJob = new TagCreateJob(remoteTag, this);
            createJob->setMergeIfExisting(true);
            connect(createJob, &KJob::result, this, &TagSync::onCreateTagDone);
            connect(createJob, &KJob::result, this, &TagSync::onJobDone);
        }
    }
    for (const Tag &tag : std::as_const(tagById)) {
        // Removed remotely, unset rid
        Tag copy(tag);
        copy.setRemoteId(QByteArray(""));
        auto modJob = new TagModifyJob(copy, this);
        connect(modJob, &KJob::result, this, &TagSync::onJobDone);
    }
    checkDone();
}

void TagSync::onCreateTagDone(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "ItemFetch failed: " << job->errorString();
        return;
    }

    Akonadi::Tag tag = static_cast<Akonadi::TagCreateJob *>(job)->tag();
    const Item::List remoteMembers = mRidMemberMap.value(QString::fromLatin1(tag.remoteId()));
    for (Item item : remoteMembers) {
        item.setTag(tag);
        auto modJob = new ItemModifyJob(item, this);
        connect(modJob, &KJob::result, this, &TagSync::onJobDone);
        qCDebug(AKONADICORE_LOG) << "setting tag " << item.remoteId();
    }
}

static bool containsByGidOrRid(const Item::List &items, const Item &key)
{
    return std::any_of(items.cbegin(), items.cend(), [&key](const Item &item) {
        return ((!item.gid().isEmpty() && !key.gid().isEmpty()) && (item.gid() == key.gid())) || (item.remoteId() == key.remoteId());
    });
}

void TagSync::onTagItemsFetchDone(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "ItemFetch failed: " << job->errorString();
        return;
    }

    const Akonadi::Item::List items = static_cast<Akonadi::ItemFetchJob *>(job)->items();
    const auto tag = job->property("tag").value<Akonadi::Tag>();
    const bool merge = job->property("merge").toBool();
    const Item::List remoteMembers = mRidMemberMap.value(QString::fromLatin1(tag.remoteId()));

    // add = remote - local
    Item::List toAdd;
    for (const Item &remote : remoteMembers) {
        if (!containsByGidOrRid(items, remote)) {
            toAdd << remote;
        }
    }

    // remove = local - remote
    Item::List toRemove;
    for (const Item &local : items) {
        // Skip items that have no remote id yet
        // Trying to them will only result in a conflict
        if (local.remoteId().isEmpty()) {
            continue;
        }
        if (!containsByGidOrRid(remoteMembers, local)) {
            toRemove << local;
        }
    }

    if (!merge) {
        for (Item item : std::as_const(toRemove)) {
            item.clearTag(tag);
            auto modJob = new ItemModifyJob(item, this);
            connect(modJob, &KJob::result, this, &TagSync::onJobDone);
            qCDebug(AKONADICORE_LOG) << "removing tag " << item.remoteId();
        }
    }
    for (Item item : std::as_const(toAdd)) {
        item.setTag(tag);
        auto modJob = new ItemModifyJob(item, this);
        connect(modJob, &KJob::result, this, &TagSync::onJobDone);
        qCDebug(AKONADICORE_LOG) << "setting tag " << item.remoteId();
    }
}

void TagSync::onJobDone(KJob * /*unused*/)
{
    checkDone();
}

void TagSync::slotResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Error during TagSync: " << job->errorString() << job->metaObject()->className();
        // pretend there were no errors
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
    qCDebug(AKONADICORE_LOG) << "done";
    emitResult();
}
