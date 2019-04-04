/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagmodifyjob.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "tag.h"
#include "tag_p.h"

using namespace Akonadi;

class Akonadi::TagModifyJobPrivate : public JobPrivate
{
public:
    explicit TagModifyJobPrivate(TagModifyJob *parent)
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

Tag TagModifyJob::tag() const
{
    Q_D(const TagModifyJob);
    return d->mTag;
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
        if (response->type() == Protocol::Command::FetchTags || response->type() == Protocol::Command::DeleteTag) {
            // Tag was modified, deleted or merged, we ignore the response for now
            return false;
        } else if (response->type() == Protocol::Command::ModifyTag) {
            // Done.
            d->mTag.d_ptr->resetChangeLog();
            return true;
        }
    }

    return Job::doHandleResponse(tag, response);
}

#include "moc_tagmodifyjob.cpp"
