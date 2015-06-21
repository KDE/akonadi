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

#include "tagmodifyjob.h"
#include "job_p.h"
#include "tag.h"
#include "protocolhelper_p.h"
#include "changemediator_p.h"

using namespace Akonadi;

struct Akonadi::TagModifyJobPrivate : public JobPrivate
{
    TagModifyJobPrivate(TagModifyJob *parent)
        : JobPrivate(parent)
    {
    }

    Tag mTag;
};

TagModifyJob::TagModifyJob(const Akonadi::Tag &tag, QObject *parent)
    : Job(new TagModifyJobPrivate(this), parent)
{
    Q_D(TagModifyJob);
    d->mTag = tag;
}

void TagModifyJob::doStart()
{
    Q_D(TagModifyJob);

    Protocol::ModifyTagCommand cmd(d->mTag.id());
    if (!d->mTag.remoteId().isNull()) {
        cmd.setRemoteId(d->mTag.remoteId());
    }
    if (!d->mTag.type().isEmpty()) {
        cmd.setType(d->mTag.type());
    }
    if (d->mTag.parent().isValid() && !d->mTag.isImmutable()) {
        cmd.setParentId(d->mTag.parent().id());
    }
    if (!d->mTag.removedAttributes().isEmpty()) {
        cmd.setRemovedAttributes(d->mTag.removedAttributes());
    }

    cmd.setAttributes(ProtocolHelper::attributesToProtocol(d->mTag));

    d->sendCommand(cmd);
}

bool TagModifyJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    Q_D(TagModifyJob);

    if (response.isResponse() && (response.type() == Protocol::Command::DeleteTag
            || response.type() == Protocol::Command::ModifyTag)) {
        ChangeMediator::invalidateTag(d->mTag);
        return true;
    }

    // We ignore the result for now
    if (response.isResponse() && response.type() == Protocol::Command::FetchTags) {
        return false;
    }

    return Job::doHandleResponse(tag, response);
}
