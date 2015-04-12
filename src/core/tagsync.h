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
#ifndef TAGSYNC_H
#define TAGSYNC_H

#include "akonadicore_export.h"

#include "jobs/job.h"
#include "tag.h"
#include "item.h"

class AKONADICORE_EXPORT TagSync : public Akonadi::Job
{
    Q_OBJECT
public:
    TagSync(QObject *parent = 0);
    virtual ~TagSync();

    void setFullTagList(const Akonadi::Tag::List &tags);
    void setTagMembers(const QHash<QString, Akonadi::Item::List> &ridMemberMap);

protected:
    void doStart();

private Q_SLOTS:
    void onLocalTagFetchDone(KJob *job);
    void onCreateTagDone(KJob *job);
    void onTagItemsFetchDone(KJob *job);
    void onJobDone(KJob *job);
    void slotResult(KJob *job);

private:
    void diffTags();
    void checkDone();

private:
    Akonadi::Tag::List mRemoteTags;
    Akonadi::Tag::List mLocalTags;
    bool mDeliveryDone;
    bool mTagMembersDeliveryDone;
    bool mLocalTagsFetched;
    QHash<QString, Akonadi::Item::List> mRidMemberMap;
};

#endif
