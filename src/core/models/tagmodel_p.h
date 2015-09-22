/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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
    virtual ~TagModelPrivate();

    void init(Monitor *recorder);
    void fillModel();

    void tagsFetchDone(KJob *job);
    void tagsFetched(const Akonadi::Tag::List &tags);
    void monitoredTagAdded(const Akonadi::Tag &tag);
    void monitoredTagChanged(const Akonadi::Tag &tag);
    void monitoredTagRemoved(const Akonadi::Tag &tag);

    QModelIndex indexForTag(qint64 tagId) const;
    Tag tagForIndex(const QModelIndex &index) const;

    void removeTagsRecursively(qint64 parentTag);

    Monitor *mMonitor;
    Session *mSession;

    QHash<Tag::Id /* parent */, Tag::List > mChildTags;
    QHash<Tag::Id /* tag ID */, Tag> mTags;

    QHash<Tag::Id /* missing parent */, Tag::List > mPendingTags;

protected:
    Q_DECLARE_PUBLIC(TagModel)
    TagModel *q_ptr;

};
}

#endif // AKONADI_TAGMODELPRIVATE_H
