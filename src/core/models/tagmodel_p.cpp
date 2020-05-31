/*
    Copyright (c) 2014 Daniel Vr??til <dvratil@redhat.com>

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

#include "tagmodel_p.h"
#include "tagmodel.h"

#include "monitor.h"
#include "session.h"
#include "tagfetchjob.h"

#include "akonadicore_debug.h"

#include <QTimer>

using namespace Akonadi;

TagModelPrivate::TagModelPrivate(TagModel *parent)
    : mMonitor(nullptr)
    , mSession(nullptr)
    , q_ptr(parent)
{
    // Root tag
    mTags.insert(-1, Tag());
}

void TagModelPrivate::init(Monitor *monitor)
{
    Q_Q(TagModel);

    mMonitor = monitor;
    mSession = mMonitor->session();

    q->connect(mMonitor, &Monitor::tagAdded, q, [this](const Tag &tag) { monitoredTagAdded(tag); });
    q->connect(mMonitor, &Monitor::tagChanged, q, [this](const Tag &tag) { monitoredTagChanged(tag); });
    q->connect(mMonitor, &Monitor::tagRemoved, q, [this](const Tag &tag) { monitoredTagRemoved(tag); });

    // Delay starting the job to allow unit-tests to set up fake stuff
    QTimer::singleShot(0, q, [this] { fillModel(); });
}

void TagModelPrivate::fillModel()
{
    Q_Q(TagModel);

    TagFetchJob *fetchJob = new TagFetchJob(mSession);
    fetchJob->setFetchScope(mMonitor->tagFetchScope());
    q->connect(fetchJob, &TagFetchJob::tagsReceived, q, [this](const auto &tags) { tagsFetched(tags); });
    q->connect(fetchJob, &KJob::result, q, [this](KJob *job) { tagsFetchDone(job); });
}

QModelIndex TagModelPrivate::indexForTag(const qint64 tagId) const
{
    Q_Q(const TagModel);

    if (!mTags.contains(tagId)) {
        return QModelIndex();
    }

    const Tag tag = mTags.value(tagId);
    if (!tag.isValid()) {
        return QModelIndex();
    }

    const Tag::Id parentId = tag.parent().id();
    const int row = mChildTags.value(parentId).indexOf(tag);
    if (row != -1) {
        return q->createIndex(row, 0, (int) parentId);
    }

    return QModelIndex();
}

Tag TagModelPrivate::tagForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Tag();
    }

    const Tag::Id parentId = index.internalId();
    const Tag::List &children = mChildTags.value(parentId);
    return children.value(index.row());
}

void TagModelPrivate::monitoredTagAdded(const Tag &tag)
{
    Q_Q(TagModel);

    const qint64 parentId = tag.parent().id();

    // Parent not yet in model, defer for later
    if (!mTags.contains(parentId)) {
        Tag::List &list = mPendingTags[parentId];
        list.append(tag);
        return;
    }

    Tag::List &children = mChildTags[parentId];

    q->beginInsertRows(indexForTag(parentId), children.count(), children.count());
    mTags.insert(tag.id(), tag);
    children.append(tag);
    q->endInsertRows();

    // If there are any child tags waiting for this parent, insert them
    if (mPendingTags.contains(tag.id())) {
        const Tag::List pendingChildren = mPendingTags.take(tag.id());
        for (const Tag &pendingTag : pendingChildren) {
            monitoredTagAdded(pendingTag);
        }
    }
}

void TagModelPrivate::removeTagsRecursively(qint64 tagId)
{
    const Tag tag = mTags.value(tagId);

    // Remove all children first
    const Tag::List childTags = mChildTags.take(tagId);
    for (const Tag &child : childTags) {
        removeTagsRecursively(child.id());
    }

    // Remove the actual tag
    Tag::List &siblings = mChildTags[tag.parent().id()];
    siblings.removeOne(tag);
    mTags.remove(tag.id());
}

void TagModelPrivate::monitoredTagRemoved(const Tag &tag)
{
    Q_Q(TagModel);

    if (!tag.isValid()) {
        qCWarning(AKONADICORE_LOG) << "Attempting to remove root tag?";
        return;
    }

    // Better lookup parent in our cache
    auto iter = mTags.constFind(tag.id());
    if (iter == mTags.cend()) {
        qCWarning(AKONADICORE_LOG) << "Got removal notification for unknown tag" << tag.id();
        return;
    }

    const qint64 parentId = iter->parent().id();

    const Tag::List &siblings = mChildTags[parentId];
    const int pos = siblings.indexOf(tag);
    Q_ASSERT(pos != -1);

    q->beginRemoveRows(indexForTag(parentId), pos, pos);
    removeTagsRecursively(tag.id());
    q->endRemoveRows();
}

void TagModelPrivate::monitoredTagChanged(const Tag &tag)
{
    Q_Q(TagModel);

    if (!mTags.contains(tag.id())) {
        qCWarning(AKONADICORE_LOG) << "Got change notifications for unknown tag" << tag.id();
        return;
    }

    const Tag oldTag = mTags.value(tag.id());
    // Replace existing tag in cache
    mTags.insert(tag.id(), tag);

    // Check whether the tag has been reparented
    const qint64 oldParent = oldTag.parent().id();
    const qint64 newParent = tag.parent().id();
    if (oldParent != newParent) {
        const QModelIndex sourceParent = indexForTag(oldParent);
        const int sourcePos = mChildTags.value(oldParent).indexOf(oldTag);
        const QModelIndex destParent = indexForTag(newParent);
        const int destPos = mChildTags.value(newParent).count();

        q->beginMoveRows(sourceParent, sourcePos, sourcePos, destParent, destPos);
        Tag::List &oldSiblings = mChildTags[oldParent];
        oldSiblings.removeAt(sourcePos);
        Tag::List &newSiblings = mChildTags[newParent];
        newSiblings.append(tag);
        q->endMoveRows();
    } else {
        Tag::List &children = mChildTags[oldParent];
        const int sourcePos = children.indexOf(oldTag);
        if (sourcePos != -1) {
            children[sourcePos] = tag;
        }

        const QModelIndex index = indexForTag(tag.id());
        Q_EMIT q->dataChanged(index, index);
    }
}

void TagModelPrivate::tagsFetched(const Tag::List &tags)
{
    for (const Tag &tag : tags) {
        monitoredTagAdded(tag);
    }
}

void TagModelPrivate::tagsFetchDone(KJob *job)
{
    Q_Q(TagModel);

    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorString();
        return;
    }

    if (!mPendingTags.isEmpty()) {
        qCWarning(AKONADICORE_LOG) << "Fetched all tags from server, but there are still" << mPendingTags.count() << "orphan tags:";
        for (auto it = mPendingTags.cbegin(), e = mPendingTags.cend(); it != e; ++it) {
            qCWarning(AKONADICORE_LOG) << "tagId = " << it.key() << "; with list count =" << it.value().count();
        }

        return;
    }

    Q_EMIT q->populated();
}
