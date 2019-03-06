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
#include "tag_p.h"
#include "protocolhelper_p.h"
#include "changemediator_p.h"

using namespace Akonadi;

class Akonadi::TagModifyJobPrivate : public JobPrivate
{
public:
    TagModifyJobPrivate(TagModifyJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const override;
    Tag mTag;

};

QString Akonadi::TagModifyJobPrivate::jobDebuggingString() const
{
    return QStringLiteral("Modify Tag: %1").arg(mTag.name());
}

TagModifyJob::TagModifyJob(const Akonadi::Tag &tag, QObject *parent)
    : Job(new TagModifyJobPrivate(this), parent)
{
    Q_D(TagModifyJob);
    d->mTag = tag;
}

void TagModifyJob::doStart()
{
    Q_D(TagModifyJob);

    auto cmd = Protocol::ModifyTagCommandPtr::create(d->mTag.id());
    if (!d->mTag.remoteId().isNull()) {
        cmd->setRemoteId(d->mTag.remoteId());
    }
    if (!d->mTag.type().isEmpty()) {
        cmd->setType(d->mTag.type());
    }
    if (d->mTag.parent().isValid() && !d->mTag.isImmutable()) {
        cmd->setParentId(d->mTag.parent().id());
    }
    if (!d->mTag.d_ptr->mAttributeStorage.deletedAttributes().isEmpty()) {
        cmd->setRemovedAttributes(d->mTag.d_ptr->mAttributeStorage.deletedAttributes());
    }
    if (d->mTag.d_ptr->mAttributeStorage.hasModifiedAttributes()) {
        cmd->setAttributes(ProtocolHelper::attributesToProtocol(d->mTag.d_ptr->mAttributeStorage.modifiedAttributes()));
    }

    d->sendCommand(cmd);
}

bool TagModifyJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(TagModifyJob);

    if (response->isResponse()) {
        if (response->type() == Protocol::Command::FetchTags) {
            // Tag was modified, we ignore the response for now
            return false;
        } else if (response->type() == Protocol::Command::DeleteTag) {
            // The tag was deleted/merged
            return false;
        } else if (response->type() == Protocol::Command::ModifyTag) {
            // Done.
            d->mTag.d_ptr->resetChangeLog();
            return true;
        }
    }

    return Job::doHandleResponse(tag, response);
}
