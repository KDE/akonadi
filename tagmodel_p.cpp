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
#include <akonadi/tagfetchjob.h>
#include <QDebug>

using namespace Akonadi;

TagModelPrivate::TagModelPrivate(TagModel *parent)
    : mMonitor(0)
    , q_ptr(parent)
{
    // Root tag
    mTags.insert(0, Tag(0));
}

TagModelPrivate::~TagModelPrivate()
{
}

void TagModelPrivate::init(Monitor *monitor)
{
    Q_Q(TagModel);

    mMonitor = monitor;

    q->connect(mMonitor, SIGNAL(tagAdded(Akonadi::Tag)),
               q, SLOT(monitoredTagAdded(Akonadi::Tag)));
    q->connect(mMonitor, SIGNAL(tagChanged(Akonadi::Tag)),
               q, SLOT(monitoredTagChanged(Akonadi::Tag)));
    q->connect(mMonitor, SIGNAL(tagRemoved(Akonadi::Tag)),
               q, SLOT(monitoredTagRemoved(Akonadi::Tag)));

    TagFetchJob *fetchJob = new TagFetchJob(q);
    fetchJob->setFetchScope(mMonitor->tagFetchScope());
    q->connect(fetchJob, SIGNAL(tagsReceived(Akonadi::Tag::List)),
               q, SLOT(tagsFetched(Akonadi::Tag::List)));
    q->connect(fetchJob, SIGNAL(finished(KJob*)),
               q, SLOT(tagsFetchDone(KJob*)));
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
    return children.at(index.row());
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
        Tag::List &children = mChildTags[tag.id()];
        q->beginInsertRows(indexForTag(tag.id()), 0, pendingChildren.count() - 1);
        Q_FOREACH (const Tag &child, pendingChildren) {
            mTags.insert(child.id(), child);
            children.append(child);
        }
        q->endInsertRows();
    }
}

void TagModelPrivate::removeTagsRecursively(qint64 tagId)
{
    const Tag tag = mTags.value(tagId);

    // Remove all children first
    const Tag::List childTags = mChildTags.take(tagId);
    Q_FOREACH (const Tag &child, childTags) {
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

    // Better lookup parent in our cache
    qint64 parentId = mTags.value(tag.id()).parent().id();
    if (parentId == -1) {
        kWarning() << "Got removal notification for unknown tag" << tag.id();
        return;
    }

    Tag::List &siblings = mChildTags[parentId];
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
        kWarning() << "Got change notifications for unknown tag" << tag.id();
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
        q->dataChanged(index, index);
    }
}

void TagModelPrivate::tagsFetched(const Tag::List &tags)
{
    Q_FOREACH (const Tag &tag, tags) {
        monitoredTagAdded(tag);
    }
}

void TagModelPrivate::tagsFetchDone(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
        return;
    }

    if (!mPendingTags.isEmpty()) {
        kWarning() << "Fetched all tags from server, but there are still" << mPendingTags.count() << "orphan tags";
        return;
    }
}
