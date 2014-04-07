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

#include "imapparser_p.h"
#include "itemfetchscope.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "searchquery.h"

#include <QtCore/QTimer>
#include <QThreadStorage>

using namespace Akonadi;

class Akonadi::ItemSearchJobPrivate : public JobPrivate
{
public:
    ItemSearchJobPrivate(ItemSearchJob *parent, const SearchQuery &query)
        : JobPrivate(parent)
        , mQuery(query)
        , mRecursive(false)
        , mRemote(false)
        , mEmitTimer(0)
    {
    }

    void init()
    {
        Q_Q(ItemSearchJob);
        mEmitTimer = new QTimer(q);
        mEmitTimer->setSingleShot(true);
        mEmitTimer->setInterval(100);
        q->connect(mEmitTimer, SIGNAL(timeout()), q, SLOT(timeout()));
        q->connect(q, SIGNAL(result(KJob*)), q, SLOT(timeout()));
    }

    void timeout()
    {
        Q_Q(Akonadi::ItemSearchJob);

        mEmitTimer->stop(); // in case we are called by result()
        if (!mPendingItems.isEmpty()) {
            if (!q->error()) {
                emit q->itemsReceived(mPendingItems);
            }
            mPendingItems.clear();
        }
    }

    Q_DECLARE_PUBLIC(ItemSearchJob)

    SearchQuery mQuery;
    Collection::List mCollections;
    QStringList mMimeTypes;
    bool mRecursive;
    bool mRemote;
    ItemFetchScope mFetchScope;

    Item::List mItems;
    Item::List mPendingItems; // items pending for emitting itemsReceived()

    QTimer *mEmitTimer;
};

QThreadStorage<Session *> instances;

static Session *defaultSearchSession()
{
    if (!instances.hasLocalData()) {
        const QByteArray sessionName = Session::defaultSession()->sessionId() + "-SearchSession";
        instances.setLocalData(new Session(sessionName));
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

ItemSearchJob::ItemSearchJob(const SearchQuery &query, QObject *parent)
    : Job(new ItemSearchJobPrivate(this, query), sessionForJob(parent))
{
    Q_D(ItemSearchJob);

    d->init();
}

ItemSearchJob::ItemSearchJob(const QString &query, QObject *parent)
    : Job(new ItemSearchJobPrivate(this, SearchQuery::fromJSON(query.toUtf8())), sessionForJob(parent))
{
    Q_D(ItemSearchJob);

    d->init();
}

ItemSearchJob::~ItemSearchJob()
{
}

void ItemSearchJob::setQuery(const QString &query)
{
    Q_D(ItemSearchJob);

    d->mQuery = SearchQuery::fromJSON(query.toUtf8());
}

void ItemSearchJob::setQuery(const SearchQuery &query)
{
    Q_D(ItemSearchJob);

    d->mQuery = query;
}

void ItemSearchJob::setFetchScope(const ItemFetchScope &fetchScope)
{
    Q_D(ItemSearchJob);

    d->mFetchScope = fetchScope;
}

ItemFetchScope &ItemSearchJob::fetchScope()
{
    Q_D(ItemSearchJob);

    return d->mFetchScope;
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

    QByteArray command = d->newTag() + " SEARCH ";
    if (!d->mMimeTypes.isEmpty()) {
        command += "MIMETYPE (" + d->mMimeTypes.join(QLatin1String(" ")).toLatin1() + ") ";
    }
    if (!d->mCollections.isEmpty()) {
        command += "COLLECTIONS (";
        Q_FOREACH (const Collection &collection, d->mCollections) {
            command += QByteArray::number(collection.id()) + ' ';
        }
        command += ") ";
    }
    if (d->mRecursive) {
        command += "RECURSIVE ";
    }
    if (d->mRemote) {
        command += "REMOTE ";
    }

    command += "QUERY " + ImapParser::quote(d->mQuery.toJSON());
    command += ' ' + ProtocolHelper::itemFetchScopeToByteArray(d->mFetchScope);
    command += '\n';
    d->writeData(command);
}

void ItemSearchJob::doHandleResponse(const QByteArray &tag, const QByteArray &data)
{
    Q_D(ItemSearchJob);

    if (tag == "*") {
        int begin = data.indexOf("SEARCH");
        if (begin >= 0) {

            // split fetch response into key/value pairs
            QList<QByteArray> fetchResponse;
            ImapParser::parseParenthesizedList(data, fetchResponse, begin + 7);

            Item item;
            ProtocolHelper::parseItemFetchResult(fetchResponse, item);
            if (!item.isValid()) {
                return;
            }

            d->mItems.append(item);
            d->mPendingItems.append(item);
            if (!d->mEmitTimer->isActive()) {
                d->mEmitTimer->start();
            }
            return;
        }
    }
    kDebug() << "Unhandled response: " << tag << data;
}

Item::List ItemSearchJob::items() const
{
    Q_D(const ItemSearchJob);

    return d->mItems;
}

QUrl ItemSearchJob::akonadiItemIdUri()
{
    return QUrl(QLatin1String("http://akonadi-project.org/ontologies/aneo#akonadiItemId"));
}

#include "moc_itemsearchjob.cpp"
