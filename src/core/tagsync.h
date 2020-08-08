/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef TAGSYNC_H
#define TAGSYNC_H

#include "akonadicore_export.h"

#include "tag.h"
#include "item.h"

#include <KJob>

namespace Akonadi
{

class AKONADICORE_EXPORT TagSync : public KJob
{
    Q_OBJECT
public:
    explicit TagSync(QObject *parent = nullptr);
    ~TagSync() override;

    void setFullTagList(const Akonadi::Tag::List &tags);
    void setTagMembers(const QHash<QString, Akonadi::Item::List> &ridMemberMap);

protected:
    void start() override;

private:
    void onTagItemsFetchDone(const Tag &tag, const Item::List &items, bool merge);
    void diffTags();
    void checkDone();

private:
    Akonadi::Tag::List mRemoteTags;
    Akonadi::Tag::List mLocalTags;
    bool mDeliveryDone;
    bool mTagMembersDeliveryDone;
    bool mLocalTagsFetched;
    QHash<QString, Akonadi::Item::List> mRidMemberMap;
    int mTasks = 0;
};

} // namespace Akonadi

#endif
