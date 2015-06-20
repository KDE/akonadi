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

#include "tagdeletejob.h"
#include "job_p.h"
#include "protocolhelper_p.h"

using namespace Akonadi;

struct Akonadi::TagDeleteJobPrivate : public JobPrivate
{
    TagDeleteJobPrivate(TagDeleteJob *parent)
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

    d->sendCommand(Protocol::DeleteTagCommand(ProtocolHelper::entitySetToScope(d->mTagsToRemove)));
}

bool TagDeleteJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    if (!response.isResponse() || response.type() != Protocol::Command::DeleteTag) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

Tag::List TagDeleteJob::tags() const
{
    Q_D(const TagDeleteJob);
    return d->mTagsToRemove;
}
