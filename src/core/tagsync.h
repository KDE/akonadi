/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef TAGSYNC_H
#define TAGSYNC_H

#include "akonadicore_export.h"

#include "jobs/job.h"
#include "tag.h"
#include "item.h"

namespace Akonadi
{

class AKONADICORE_EXPORT TagSync : public Akonadi::Job
{
    Q_OBJECT
public:
    explicit TagSync(QObject *parent = nullptr);
    ~TagSync() override;

    void setFullTagList(const Akonadi::Tag::List &tags);
    void setTagMembers(const QHash<QString, Akonadi::Item::List> &ridMemberMap);

protected:
    void doStart() override;

private Q_SLOTS:
    void onLocalTagFetchDone(KJob *job);
    void onCreateTagDone(KJob *job);
    void onTagItemsFetchDone(KJob *job);
    void onJobDone(KJob *job);
    void slotResult(KJob *job) override;

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

}

#endif
