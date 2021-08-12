/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagfetchjob.h"
#include "attributefactory.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "tagfetchscope.h"
#include <QTimer>

using namespace Akonadi;
using namespace std::chrono_literals;
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
        mEmitTimer->setInterval(100ms);
        q->connect(mEmitTimer, &QTimer::timeout, q, [this]() {
            timeout();
        });
    }

    void aboutToFinish() override
    {
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
#include <chrono>

using namespace std::chrono_literals;
