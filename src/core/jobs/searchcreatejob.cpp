
/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "searchcreatejob.h"

#include "job_p.h"
#include "protocolhelper_p.h"
#include "searchquery.h"

#include "private/protocol_p.h"

using namespace Akonadi;

class Akonadi::SearchCreateJobPrivate : public JobPrivate
{
public:
    SearchCreateJobPrivate(const QString &name, const SearchQuery &query, SearchCreateJob *parent)
        : JobPrivate(parent)
        , mName(name)
        , mQuery(query)
    {
    }

    const QString mName;
    const SearchQuery mQuery;
    QStringList mMimeTypes;
    QList<Collection> mCollections;
    bool mRecursive = false;
    bool mRemote = false;
    Collection mCreatedCollection;

    // JobPrivate interface
public:
    QString jobDebuggingString() const override;
};

QString Akonadi::SearchCreateJobPrivate::jobDebuggingString() const
{
    QString str = QStringLiteral("Name :%1 ").arg(mName);
    if (mRecursive) {
        str += QStringLiteral("Recursive ");
    }
    if (mRemote) {
        str += QStringLiteral("Remote");
    }
    return str;
}

SearchCreateJob::SearchCreateJob(const QString &name, const SearchQuery &searchQuery, QObject *parent)
    : Job(new SearchCreateJobPrivate(name, searchQuery, this), parent)
{
}

SearchCreateJob::~SearchCreateJob()
{
}

void SearchCreateJob::setSearchCollections(const QList<Collection> &collections)
{
    Q_D(SearchCreateJob);

    d->mCollections = collections;
}

QList<Collection> SearchCreateJob::searchCollections() const
{
    return d_func()->mCollections;
}

void SearchCreateJob::setSearchMimeTypes(const QStringList &mimeTypes)
{
    Q_D(SearchCreateJob);

    d->mMimeTypes = mimeTypes;
}

QStringList SearchCreateJob::searchMimeTypes() const
{
    return d_func()->mMimeTypes;
}

void SearchCreateJob::setRecursive(bool recursive)
{
    Q_D(SearchCreateJob);

    d->mRecursive = recursive;
}

bool SearchCreateJob::isRecursive() const
{
    return d_func()->mRecursive;
}

void SearchCreateJob::setRemoteSearchEnabled(bool enabled)
{
    Q_D(SearchCreateJob);

    d->mRemote = enabled;
}

bool SearchCreateJob::isRemoteSearchEnabled() const
{
    return d_func()->mRemote;
}

void SearchCreateJob::doStart()
{
    Q_D(SearchCreateJob);

    auto cmd = Protocol::StoreSearchCommandPtr::create();
    cmd->setName(d->mName);
    cmd->setQuery(QString::fromUtf8(d->mQuery.toJSON()));
    cmd->setMimeTypes(d->mMimeTypes);
    cmd->setRecursive(d->mRecursive);
    cmd->setRemote(d->mRemote);
    if (!d->mCollections.isEmpty()) {
        QList<qint64> ids;
        ids.reserve(d->mCollections.size());
        for (const Collection &col : std::as_const(d->mCollections)) {
            ids << col.id();
        }
        cmd->setQueryCollections(ids);
    }

    d->sendCommand(cmd);
}

Akonadi::Collection SearchCreateJob::createdCollection() const
{
    Q_D(const SearchCreateJob);
    return d->mCreatedCollection;
}

bool SearchCreateJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(SearchCreateJob);
    if (response->isResponse() && response->type() == Protocol::Command::FetchCollections) {
        d->mCreatedCollection = ProtocolHelper::parseCollection(Protocol::cmdCast<Protocol::FetchCollectionsResponse>(response));
        return false;
    }

    if (response->isResponse() && response->type() == Protocol::Command::StoreSearch) {
        return true;
    }

    return Job::doHandleResponse(tag, response);
}
