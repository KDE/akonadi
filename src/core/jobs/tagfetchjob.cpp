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
    TagFetchJobPrivate(TagFetchJob *parent)
        : JobPrivate(parent)
        , mEmitTimer(0)
    {
    }

    void init()
    {
        Q_Q(TagFetchJob);
        mEmitTimer = new QTimer(q);
        mEmitTimer->setSingleShot(true);
        mEmitTimer->setInterval(100);
        q->connect(mEmitTimer, SIGNAL(timeout()), q, SLOT(timeout()));
    }

    void aboutToFinish()
    {
      timeout();
    }

    void timeout()
    {
        Q_Q(TagFetchJob);
        mEmitTimer->stop(); // in case we are called by result()
        if (!mPendingTags.isEmpty()) {
            if (!q->error()) {
                emit q->tagsReceived(mPendingTags);
            }
            mPendingTags.clear();
        }
    }

    Q_DECLARE_PUBLIC(TagFetchJob)

    Tag::List mRequestedTags;
    Tag::List mResultTags;
    Tag::List mPendingTags; // items pending for emitting itemsReceived()
    QTimer *mEmitTimer;
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
    Q_FOREACH (Tag::Id id, ids) {
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

    QByteArray command = d->newTag();
    if (d->mRequestedTags.isEmpty()) {
        command += " UID TAGFETCH 1:*";
    } else {
        try {
            command += ProtocolHelper::tagSetToByteArray(d->mRequestedTags, "TAGFETCH");
        } catch (const Exception &e) {
            setError(Job::Unknown);
            setErrorText(QString::fromUtf8(e.what()));
            emitResult();
            return;
        }
    }
    command += " " + ProtocolHelper::tagFetchScopeToByteArray(d->mFetchScope) + "\n";

    d->writeData(command);
}

void TagFetchJob::doHandleResponse(const QByteArray &tag, const QByteArray &data)
{
    Q_D(TagFetchJob);

    if (tag == "*") {
        int begin = data.indexOf("TAGFETCH");
        if (begin >= 0) {
            // split fetch response into key/value pairs
            QList<QByteArray> fetchResponse;
            ImapParser::parseParenthesizedList(data, fetchResponse, begin + 8);

            Tag tag;
            ProtocolHelper::parseTagFetchResult(fetchResponse, tag);

            if (tag.isValid()) {
                d->mResultTags.append(tag);
                d->mPendingTags.append(tag);
                if (!d->mEmitTimer->isActive()) {
                    d->mEmitTimer->start();
                }
            }
            return;
        }
    }
    qDebug() << "Unhandled response: " << tag << data;
}

Tag::List TagFetchJob::tags() const
{
    Q_D(const TagFetchJob);
    return d->mResultTags;
}

#include "moc_tagfetchjob.cpp"
