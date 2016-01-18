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

#include "tagcreatejob.h"
#include "job_p.h"
#include "tag.h"
#include "protocolhelper_p.h"
#include "akonadicore_debug.h"
#include <KLocalizedString>

using namespace Akonadi;

struct Akonadi::TagCreateJobPrivate : public JobPrivate {
    TagCreateJobPrivate(TagCreateJob *parent)
        : JobPrivate(parent)
        , mMerge(false)
    {
    }

    Tag mTag;
    Tag mResultTag;
    bool mMerge;
};

TagCreateJob::TagCreateJob(const Akonadi::Tag &tag, QObject *parent)
    : Job(new TagCreateJobPrivate(this), parent)
{
    Q_D(TagCreateJob);
    d->mTag = tag;
}

void TagCreateJob::setMergeIfExisting(bool merge)
{
    Q_D(TagCreateJob);
    d->mMerge = merge;
}

void TagCreateJob::doStart()
{
    Q_D(TagCreateJob);

    if (d->mTag.gid().isEmpty()) {
        qCWarning(AKONADICORE_LOG) << "The gid of a new tag must not be empty";
        setError(Job::Unknown);
        setErrorText(i18n("Failed to create tag."));
        emitResult();
        return;
    }

    Protocol::CreateTagCommand cmd;
    cmd.setGid(d->mTag.gid());
    cmd.setMerge(d->mMerge);
    cmd.setType(d->mTag.type());
    cmd.setRemoteId(d->mTag.remoteId());
    cmd.setParentId(d->mTag.parent().id());
    cmd.setAttributes(ProtocolHelper::attributesToProtocol(d->mTag));
    d->sendCommand(cmd);
}

bool TagCreateJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    Q_D(TagCreateJob);

    if (response.isResponse() && response.type() == Protocol::Command::FetchTags) {
        d->mResultTag = ProtocolHelper::parseTagFetchResult(response);
        return false;
    }

    if (response.isResponse() && response.type() == Protocol::Command::CreateTag) {
        return true;
    }

    return Job::doHandleResponse(tag, response);
}

Tag TagCreateJob::tag() const
{
    Q_D(const TagCreateJob);
    return d->mResultTag;
}
