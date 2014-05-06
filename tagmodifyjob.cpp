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

    QList<QByteArray> list;
    if (!d->mTag.remoteId().isEmpty()) {
        list << "REMOTEID";
        list << ImapParser::quote(d->mTag.remoteId());
    }
    if (!d->mTag.type().isEmpty()) {
        list << "MIMETYPE";
        list << ImapParser::quote(d->mTag.type());
    }
    if (d->mTag.parent().isValid() && !d->mTag.isImmutable()) {
        list << "PARENT";
        list << QString::number(d->mTag.parent().id()).toLatin1();
    }

    QByteArray command = d->newTag();
    try {
        command += ProtocolHelper::tagSetToByteArray(Tag::List() << d->mTag, "TAGSTORE");
    } catch (const std::exception &e) {
        setError(Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }
    command += " (";
    command += ImapParser::join(list, " ");
    command += " ";

    if (!d->mTag.attributes().isEmpty()) {
        command += ProtocolHelper::attributesToByteArray(d->mTag, false);
    }
    if (!d->mTag.removedAttributes().isEmpty()) {
        QList<QByteArray> l;
        Q_FOREACH (const QByteArray &attr, d->mTag.removedAttributes()) {
            l << '-' + attr;
        }
        command += ImapParser::join(l, " ");
    }

    command += ")";

    d->writeData(command);
}

void TagModifyJob::doHandleResponse(const QByteArray &tag, const QByteArray &data)
{
    Q_D(TagModifyJob);
    Q_UNUSED(tag);

    if (data.startsWith("OK")) {     //krazy:exclude=strings
        ChangeMediator::invalidateTag(d->mTag);
        emitResult();
    }
}
