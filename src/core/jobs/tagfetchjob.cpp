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

#include "tagfetchjob.h"
#include "job_p.h"
#include "tag.h"
#include "protocolhelper_p.h"
#include "tagfetchscope.h"
#include "attributefactory.h"
#include <QTimer>

using namespace Akonadi;

class Akonadi::TagFetchJobPrivate : public JobPrivate
{
public:
    explicit TagFetchJobPrivate(TagFetchJob *parent)
        : JobPrivate(parent)
    {
    }

    void init()
    {
        Q_Q(TagFetchJob);
        mEmitTimer = new QTimer(q);
        mEmitTimer->setSingleShot(true);
        mEmitTimer->setInterval(100);
        q->connect(mEmitTimer, &QTimer::timeout, q, [this]() { timeout(); });
    }

    void aboutToFinish() override {
        timeout();
    }

    void timeout()
    {
        Q_Q(TagFetchJob);
        mEmitTimer->stop(); // in case we are called by result()
        if (!mPendingTags.isEmpty()) {
            if (!q->error()) {
                Q_EMIT q->tagsReceived(mPendingTags);
            }
            mPendingTags.clear();
        }
    }

    Q_DECLARE_PUBLIC(TagFetchJob)

    Tag::List mRequestedTags;
    Tag::List mResultTags;
    Tag::List mPendingTags; // items pending for emitting itemsReceived()
    QTimer *mEmitTimer = nullptr;
    TagFetchScope mFetchScope;
};

TagFetchJob::TagFetchJob(QObject *parent)
    : Job(new TagFetchJobPrivate(this), parent)
{
    Q_D(TagFetchJob);
    d->init();
}

TagFetchJob::TagFetchJob(const Tag &tag, QObject *parent)
    : Job(new TagFetchJobPrivate(this), parent)
{
    Q_D(TagFetchJob);
    d->init();
    d->mRequestedTags << tag;
}

TagFetchJob::TagFetchJob(const Tag::List &tags, QObject *parent)
    : Job(new TagFetchJobPrivate(this), parent)
{
    Q_D(TagFetchJob);
    d->init();
    d->mRequestedTags = tags;
}

TagFetchJob::TagFetchJob(const QList<Tag::Id> &ids, QObject *parent)
    : Job(new TagFetchJobPrivate(this), parent)
{
    Q_D(TagFetchJob);
    d->init();
    for (Tag::Id id : ids) {
        d->mRequestedTags << Tag(id);
    }
}

void TagFetchJob::setFetchScope(const TagFetchScope &fetchScope)
{
    Q_D(TagFetchJob);
    d->mFetchScope = fetchScope;
}

TagFetchScope &TagFetchJob::fetchScope()
{
    Q_D(TagFetchJob);
    return d->mFetchScope;
}

void TagFetchJob::doStart()
{
    Q_D(TagFetchJob);

    Protocol::FetchTagsCommandPtr cmd;
    if (d->mRequestedTags.isEmpty()) {
        cmd = Protocol::FetchTagsCommandPtr::create(Scope(ImapInterval(1, 0)));
    } else {
        try {
            cmd = Protocol::FetchTagsCommandPtr::create(ProtocolHelper::entitySetToScope(d->mRequestedTags));
        } catch (const Exception &e) {
            setError(Job::Unknown);
            setErrorText(QString::fromUtf8(e.what()));
            emitResult();
            return;
        }
    }
    cmd->setFetchScope(ProtocolHelper::tagFetchScopeToProtocol(d->mFetchScope));

    d->sendCommand(cmd);
}

bool TagFetchJob::doHandleResponse(qint64 _tag, const Protocol::CommandPtr &response)
{
    Q_D(TagFetchJob);

    if (!response->isResponse() || response->type() != Protocol::Command::FetchTags) {
        return Job::doHandleResponse(_tag, response);
    }

    const auto &resp = Protocol::cmdCast<Protocol::FetchTagsResponse>(response);
    // Invalid tag in response marks the last response
    if (resp.id() < 0) {
        return true;
    }

    const Tag tag = ProtocolHelper::parseTagFetchResult(resp);
    d->mResultTags.append(tag);
    d->mPendingTags.append(tag);
    if (!d->mEmitTimer->isActive()) {
        d->mEmitTimer->start();
    }

    return false;
}

Tag::List TagFetchJob::tags() const
{
    Q_D(const TagFetchJob);
    return d->mResultTags;
}

#include "moc_tagfetchjob.cpp"
