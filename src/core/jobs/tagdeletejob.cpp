/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagdeletejob.h"
#include "job_p.h"
#include "protocolhelper_p.h"

using namespace Akonadi;

class Akonadi::TagDeleteJobPrivate : public JobPrivate
{
public:
    explicit TagDeleteJobPrivate(TagDeleteJob *parent)
        : JobPrivate(parent)
    {
    }

    Tag::List mTagsToRemove;
};

TagDeleteJob::TagDeleteJob(const Akonadi::Tag &tag, QObject *parent)
    : Job(new TagDeleteJobPrivate(this), parent)
{
    Q_D(TagDeleteJob);
    d->mTagsToRemove << tag;
}

TagDeleteJob::TagDeleteJob(const Tag::List &tags, QObject *parent)
    : Job(new TagDeleteJobPrivate(this), parent)
{
    Q_D(TagDeleteJob);
    d->mTagsToRemove = tags;
}

void TagDeleteJob::doStart()
{
    Q_D(TagDeleteJob);

    d->sendCommand(Protocol::DeleteTagCommandPtr::create(ProtocolHelper::entitySetToScope(d->mTagsToRemove)));
}

bool TagDeleteJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::DeleteTag) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

Tag::List TagDeleteJob::tags() const
{
    Q_D(const TagDeleteJob);
    return d->mTagsToRemove;
}
