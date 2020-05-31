/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "itemsearchjob.h"

#include "itemfetchscope.h"
#include "tagfetchscope.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "searchquery.h"
#include "private/protocol_p.h"


#include <QCoreApplication>
#include <QTimer>
#include <QThreadStorage>

using namespace Akonadi;

class Akonadi::ItemSearchJobPrivate : public JobPrivate
{
public:
    ItemSearchJobPrivate(ItemSearchJob *parent, const SearchQuery &query)
        : JobPrivate(parent)
        , mQuery(query)
    {
        mEmitTimer.setSingleShot(true);
        mEmitTimer.setInterval(std::chrono::milliseconds{100});
        QObject::connect(&mEmitTimer, &QTimer::timeout, q_ptr, [this]() { timeout(); });
    }

    void aboutToFinish() override
    {
        timeout();
    }

    void timeout()
    {
        Q_Q(Akonadi::ItemSearchJob);

        mEmitTimer.stop(); // in case we are called by result()
        if (!mPendingItems.isEmpty()) {
            if (!q->error()) {
                Q_EMIT q->itemsReceived(mPendingItems);
            }
            mPendingItems.clear();
        }
    }
    QString jobDebuggingString() const override
    {
        QStringList flags;
        if (mRecursive) {
            flags.append(QStringLiteral("recursive"));
        }
        if (mRemote) {
            flags.append(QStringLiteral("remote"));
        }
        if (mCollections.isEmpty()) {
            flags.append(QStringLiteral("all collections"));
        } else {
            flags.append(QStringLiteral("%1 collections").arg(mCollections.count()));
        }
        return QStringLiteral("%1,json=%2").arg(flags.join(QLatin1Char(',')), QString::fromUtf8(mQuery.toJSON()));
    }

    Q_DECLARE_PUBLIC(ItemSearchJob)

    SearchQuery mQuery;
    Collection::List mCollections;
    QStringList mMimeTypes;
    bool mRecursive = false;
    bool mRemote = false;
    ItemFetchScope mItemFetchScope;
    TagFetchScope mTagFetchScope;

    Item::List mItems;
    Item::List mPendingItems; // items pending for emitting itemsReceived()

    QTimer mEmitTimer;
};

QThreadStorage<Session *> instances;

static void cleanupDefaultSearchSession()
{
    instances.setLocalData(nullptr);
}

static Session *defaultSearchSession()
{
    if (!instances.hasLocalData()) {
        const QByteArray sessionName = Session::defaultSession()->sessionId() + "-SearchSession";
        instances.setLocalData(new Session(sessionName));
        qAddPostRoutine(cleanupDefaultSearchSession);
    }
    return instances.localData();
}

static QObject *sessionForJob(QObject *parent)
{
    if (qobject_cast<Job *>(parent) || qobject_cast<Session *>(parent)) {
        return parent;
    }
    return defaultSearchSession();
}

ItemSearchJob::ItemSearchJob(QObject *parent)
    : Job(new ItemSearchJobPrivate(this, SearchQuery()), sessionForJob(parent))
{}

ItemSearchJob::ItemSearchJob(const SearchQuery &query, QObject *parent)
    : Job(new ItemSearchJobPrivate(this, query), sessionForJob(parent))
{}

ItemSearchJob::~ItemSearchJob() = default;

void ItemSearchJob::setQuery(const SearchQuery &query)
{
    Q_D(ItemSearchJob);

    d->mQuery = query;
}

void ItemSearchJob::setFetchScope(const ItemFetchScope &fetchScope)
{
    Q_D(ItemSearchJob);

    d->mItemFetchScope = fetchScope;
}

ItemFetchScope &ItemSearchJob::fetchScope()
{
    Q_D(ItemSearchJob);

    return d->mItemFetchScope;
}

void ItemSearchJob::setTagFetchScope(const TagFetchScope &fetchScope)
{
    Q_D(ItemSearchJob);

    d->mTagFetchScope = fetchScope;
}

TagFetchScope &ItemSearchJob::tagFetchScope()
{
    Q_D(ItemSearchJob);

    return d->mTagFetchScope;
}

void ItemSearchJob::setSearchCollections(const Collection::List &collections)
{
    Q_D(ItemSearchJob);

    d->mCollections = collections;
}

Collection::List ItemSearchJob::searchCollections() const
{
    return d_func()->mCollections;
}

void ItemSearchJob::setMimeTypes(const QStringList &mimeTypes)
{
    Q_D(ItemSearchJob);

    d->mMimeTypes = mimeTypes;
}

QStringList ItemSearchJob::mimeTypes() const
{
    return d_func()->mMimeTypes;
}

void ItemSearchJob::setRecursive(bool recursive)
{
    Q_D(ItemSearchJob);

    d->mRecursive = recursive;
}

bool ItemSearchJob::isRecursive() const
{
    return d_func()->mRecursive;
}

void ItemSearchJob::setRemoteSearchEnabled(bool enabled)
{
    Q_D(ItemSearchJob);

    d->mRemote = enabled;
}

bool ItemSearchJob::isRemoteSearchEnabled() const
{
    return d_func()->mRemote;
}

void ItemSearchJob::doStart()
{
    Q_D(ItemSearchJob);

    auto cmd = Protocol::SearchCommandPtr::create();
    cmd->setMimeTypes(d->mMimeTypes);
    if (!d->mCollections.isEmpty()) {
        QVector<qint64> ids;
        ids.reserve(d->mCollections.size());
        for (const Collection &col : qAsConst(d->mCollections)) {
            ids << col.id();
        }
        cmd->setCollections(ids);
    }
    cmd->setRecursive(d->mRecursive);
    cmd->setRemote(d->mRemote);
    cmd->setQuery(QString::fromUtf8(d->mQuery.toJSON()));
    cmd->setItemFetchScope(ProtocolHelper::itemFetchScopeToProtocol(d->mItemFetchScope));
    cmd->setTagFetchScope(ProtocolHelper::tagFetchScopeToProtocol(d->mTagFetchScope));

    d->sendCommand(cmd);
}

bool ItemSearchJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(ItemSearchJob);

    if (response->isResponse() && response->type() == Protocol::Command::FetchItems) {
        const Item item = ProtocolHelper::parseItemFetchResult(
            Protocol::cmdCast<Protocol::FetchItemsResponse>(response));
        if (!item.isValid()) {
            return false;
        }
        d->mItems.append(item);
        d->mPendingItems.append(item);
        if (!d->mEmitTimer.isActive()) {
            d->mEmitTimer.start();
        }

        return false;
    }

    if (response->isResponse() && response->type() == Protocol::Command::Search) {
        return true;
    }

    return Job::doHandleResponse(tag, response);
}

Item::List ItemSearchJob::items() const
{
    Q_D(const ItemSearchJob);

    return d->mItems;
}

#include "moc_itemsearchjob.cpp"
