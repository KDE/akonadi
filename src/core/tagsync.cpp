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
#include "interface.h"
#include "itemfetchscope.h"
#include "tagfetchscope.h"
#include "akranges.h"

using namespace Akonadi;
using namespace AkRanges;

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
    : KJob(parent),
      mDeliveryDone(false),
      mTagMembersDeliveryDone(false),
      mLocalTagsFetched(false)
{

}

TagSync::~TagSync() = default;

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

void TagSync::start()
{
    qCDebug(AKONADICORE_LOG) << "START";
    //This should include all tags, including the ones that don't have a remote id
    TagFetchOptions options;
    options.fetchScope().setFetchRemoteId(true);
    Akonadi::fetchAllTags(options).then(
        [this](const Tag::List &tags) {
            mLocalTags = tags;
            mLocalTagsFetched = true;
            diffTags();
        },
        [](const Akonadi::Error &error) {
            qCWarning(AKONADICORE_LOG) << "Error during TagSync:" << error;
        });
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
            ItemFetchOptions options;
            options.itemFetchScope().setFetchGid(true);
            Akonadi::fetchItemsForTag(tag, options).then(
                [this, tag](const Item::List &items) {
                    onTagItemsFetchDone(tag, items, false);
                    --mTasks;
                    checkDone();
                },
                [this](const Akonadi::Error &error) {
                    qCWarning(AKONADICORE_LOG) << "ItemFetch failed: " << error;
                    --mTasks;
                    checkDone();
                });
            tagById.remove(tagByRid.value(remoteTag.remoteId()).id());
            ++mTasks;
        } else if (tagByGid.contains(remoteTag.gid())) {
            //Tag exists but has no rid
            //Merge members and set rid
            Tag tag = tagByGid.value(remoteTag.gid());
            tag.setRemoteId(remoteTag.remoteId());
            ItemFetchOptions options;
            options.itemFetchScope().setFetchGid(true);
            Akonadi::fetchItemsForTag(tag, options).then(
                [this, tag](const Item::List &items) {
                    onTagItemsFetchDone(tag, items, true);
                    --mTasks;
                    checkDone();
                },
                [this](const Akonadi::Error &error) {
                    qCWarning(AKONADICORE_LOG) << "ItemFetch failed:" << error;
                    --mTasks;
                    checkDone();
                });
            tagById.remove(tagByGid.value(remoteTag.gid()).id());
            ++mTasks;
        } else {
            //New tag, create
            Akonadi::createTag(remoteTag).then(
                [this](const Tag &tag) {
                    const Item::List remoteMembers = mRidMemberMap.value(QString::fromLatin1(tag.remoteId()));
                    for (Item item : remoteMembers) {
                        item.setTag(tag);
                        Akonadi::updateItem(item).then(
                            [this](const Item &/*item*/) {
                                --mTasks;
                                checkDone();
                            },
                            [this](const Akonadi::Error &/*error*/) {
                                --mTasks;
                                checkDone();
                            });
                        qCDebug(AKONADICORE_LOG) << "setting tag " << item.remoteId();
                        ++mTasks;
                    }
                    --mTasks;
                    checkDone();
                },
                [this](const Akonadi::Error &error) {
                    qCWarning(AKONADICORE_LOG) << "TagFetch failed: " << error;
                    --mTasks;
                    checkDone();
                });
            ++mTasks;
        }
    }
    Q_FOREACH (const Tag &tag, tagById) {
        //Removed remotely, unset rid
        Tag copy(tag);
        copy.setRemoteId(QByteArray(""));
        Akonadi::updateTag(copy).then(
            [this](const Tag & /*tag*/) {
                --mTasks;
                checkDone();
            },
            [this](const Akonadi::Error &error) {
                qCWarning(AKONADICORE_LOG) << "Error during TagSync:" << error;
                --mTasks;
                checkDone();
            });
        ++mTasks;
    }

    checkDone();
}

static bool containsByGidOrRid(const Item::List &items, const Item &key)
{
    return std::any_of(items.cbegin(), items.cend(), [&key](const Item &item) {
            return ((!item.gid().isEmpty() && !key.gid().isEmpty()) && (item.gid() == key.gid()))
                || (item.remoteId() == key.remoteId());
    });
}

void TagSync::onTagItemsFetchDone(const Tag &tag, const Item::List &items, bool merge)
{
    const Item::List remoteMembers = mRidMemberMap.value(QString::fromLatin1(tag.remoteId()));

    //add = remote - local
    const auto toAdd = remoteMembers
        | Views::filter([items](const Item &remote) { return !containsByGidOrRid(items, remote); })
        | Actions::toQVector;

    //remove = local - remote
    //Skip items that have no remote id yet
    //Trying to them will only result in a conflict
    const auto toRemove = items
        | Views::filter([](const Item &item) { return !item.remoteId().isEmpty(); })
        | Views::filter([remoteMembers](const Item &item) { return !containsByGidOrRid(remoteMembers, item); })
        | Actions::toQVector;

    if (!merge) {
        for (Item item : qAsConst(toRemove)) {
            item.clearTag(tag);
            Akonadi::updateItem(item).then(
                [this](const Item &/*item*/) {
                    --mTasks;
                    checkDone();
                },
                [this](const Akonadi::Error &error) {
                    qCWarning(AKONADICORE_LOG) << "Error during TagSync:" << error;
                    --mTasks;
                    checkDone();
                });
            ++mTasks;
            qCDebug(AKONADICORE_LOG) << "removing tag " << item.remoteId();
        }
    }
    for (Item item : qAsConst(toAdd)) {
        item.setTag(tag);
        Akonadi::updateItem(item).then(
            [this](const Item & /*item*/) {
                --mTasks;
                checkDone();
            },
            [this](const Akonadi::Error &error) {
                qCWarning(AKONADICORE_LOG) << "Error during TagSync:" << error;
                --mTasks;
                checkDone();
            });
        ++mTasks;
        qCDebug(AKONADICORE_LOG) << "setting tag " << item.remoteId();
    }
}

void TagSync::checkDone()
{
    if (mTasks > 0) {
        return;
    }
    qCDebug(AKONADICORE_LOG) << "done";
    emitResult();
}

