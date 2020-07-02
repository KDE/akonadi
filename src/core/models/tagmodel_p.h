/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TAGMODELPRIVATE_H
#define AKONADI_TAGMODELPRIVATE_H

#include "tag.h"

class QModelIndex;
class KJob;

namespace Akonadi
{

class Monitor;
class TagModel;
class Session;

class TagModelPrivate
{
public:
    explicit TagModelPrivate(TagModel *parent);

    void init(Monitor *recorder);
    void fillModel();

    void tagsFetchDone(KJob *job);
    void tagsFetched(const Akonadi::Tag::List &tags);
    void monitoredTagAdded(const Akonadi::Tag &tag);
    void monitoredTagChanged(const Akonadi::Tag &tag);
    void monitoredTagRemoved(const Akonadi::Tag &tag);

    Q_REQUIRED_RESULT QModelIndex indexForTag(qint64 tagId) const;
    Q_REQUIRED_RESULT Tag tagForIndex(const QModelIndex &index) const;

    void removeTagsRecursively(qint64 parentTag);

    Monitor *mMonitor = nullptr;
    Session *mSession = nullptr;

    QHash<Tag::Id /* parent */, Tag::List > mChildTags;
    QHash<Tag::Id /* tag ID */, Tag> mTags;

    QHash<Tag::Id /* missing parent */, Tag::List > mPendingTags;

protected:
    Q_DECLARE_PUBLIC(TagModel)
    TagModel *q_ptr;

};
}

#endif // AKONADI_TAGMODELPRIVATE_H
