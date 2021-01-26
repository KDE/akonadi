/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagcreatejob.h"
#include "akonadicore_debug.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "tag.h"
#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::TagCreateJobPrivate : public JobPrivate
{
public:
    explicit TagCreateJobPrivate(TagCreateJob *parent)
        : JobPrivate(parent)
    {
    }

    Tag mTag;
    Tag mResultTag;
    bool mMerge = false;
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

    auto cmd = Protocol::CreateTagCommandPtr::create();
    cmd->setGid(d->mTag.gid());
    cmd->setMerge(d->mMerge);
    cmd->setType(d->mTag.type());
    cmd->setRemoteId(d->mTag.remoteId());
    cmd->setParentId(d->mTag.parent().id());
    cmd->setAttributes(ProtocolHelper::attributesToProtocol(d->mTag));
    d->sendCommand(cmd);
}

bool TagCreateJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(TagCreateJob);

    if (response->isResponse() && response->type() == Protocol::Command::FetchTags) {
        d->mResultTag = ProtocolHelper::parseTagFetchResult(Protocol::cmdCast<Protocol::FetchTagsResponse>(response));
        return false;
    }

    if (response->isResponse() && response->type() == Protocol::Command::CreateTag) {
        return true;
    }

    return Job::doHandleResponse(tag, response);
}

Tag TagCreateJob::tag() const
{
    Q_D(const TagCreateJob);
    return d->mResultTag;
}
