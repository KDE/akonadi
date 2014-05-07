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
#include <KLocalizedString>

using namespace Akonadi;

struct Akonadi::TagCreateJobPrivate : public JobPrivate
{
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
        qWarning() << "The gid of a new tag must not be empty";
        setError(Job::Unknown);
        setErrorText(i18n("Failed to create tag."));
        emitResult();
        return;
    }

    QByteArray command = d->newTag() + " TAGAPPEND (";

    QList<QByteArray> list;
    list << "GID";
    list << ImapParser::quote(d->mTag.gid());

    if (d->mMerge) {
        list << "MERGE";
    }

    if (!d->mTag.type().isEmpty()) {
        list << "MIMETYPE";
        list << ImapParser::quote(d->mTag.type());
    }
    if (!d->mTag.remoteId().isEmpty()) {
        list << "REMOTEID";
        list << ImapParser::quote(d->mTag.remoteId());
    }
    if (d->mTag.parent().isValid()) {
        list << "PARENT";
        list << QString::number(d->mTag.parent().id()).toLatin1();
    }
    command += ImapParser::join(list, " ");
    command += " "; // list of parts
    const QByteArray attrs = ProtocolHelper::attributesToByteArray(d->mTag, false);
    if (!attrs.isEmpty()) {
        command += attrs;
    }
    command += ")";

    d->writeData(command);
}

void TagCreateJob::doHandleResponse(const QByteArray &tag, const QByteArray &data)
{
    Q_D(TagCreateJob);

    if (tag == "*") {
        int begin = data.indexOf("TAGFETCH");
        if (begin >= 0) {
            // split fetch response into key/value pairs
            QList<QByteArray> fetchResponse;
            ImapParser::parseParenthesizedList(data, fetchResponse, begin + 8);
            if (!d->mMerge) {
                //If merge is enabled there is the possibility that existing attributes etc are not valid anymore
                d->mResultTag = d->mTag;
            }
            ProtocolHelper::parseTagFetchResult(fetchResponse, d->mResultTag);
        }
    }
}

Tag TagCreateJob::tag() const
{
    Q_D(const TagCreateJob);
    return d->mResultTag;
}
