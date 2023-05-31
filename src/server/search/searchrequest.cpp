/*
    SPDX-FileCopyrightText: 2013, 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "searchrequest.h"

#include "abstractsearchplugin.h"
#include "akonadi.h"
#include "akonadiserver_search_debug.h"
#include "connection.h"
#include "searchmanager.h"
#include "searchtaskmanager.h"

using namespace Akonadi::Server;

SearchRequest::SearchRequest(const QByteArray &connectionId, SearchManager &searchManager, SearchTaskManager &agentSearchManager)
    : mConnectionId(connectionId)
    , mSearchManager(searchManager)
    , mAgentSearchManager(agentSearchManager)
{
}

SearchRequest::~SearchRequest()
{
}

QByteArray SearchRequest::connectionId() const
{
    return mConnectionId;
}

void SearchRequest::setQuery(const QString &query)
{
    mQuery = query;
}

QString SearchRequest::query() const
{
    return mQuery;
}

void SearchRequest::setCollections(const QList<qint64> &collectionsIds)
{
    mCollections = collectionsIds;
}

QList<qint64> SearchRequest::collections() const
{
    return mCollections;
}

void SearchRequest::setMimeTypes(const QStringList &mimeTypes)
{
    mMimeTypes = mimeTypes;
}

QStringList SearchRequest::mimeTypes() const
{
    return mMimeTypes;
}

void SearchRequest::setRemoteSearch(bool remote)
{
    mRemoteSearch = remote;
}

bool SearchRequest::remoteSearch() const
{
    return mRemoteSearch;
}

void SearchRequest::setStoreResults(bool storeResults)
{
    mStoreResults = storeResults;
}

QSet<qint64> SearchRequest::results() const
{
    return mResults;
}

void SearchRequest::emitResults(const QSet<qint64> &results)
{
    Q_EMIT resultsAvailable(results);
    if (mStoreResults) {
        mResults.unite(results);
    }
}

void SearchRequest::searchPlugins()
{
    const QList<AbstractSearchPlugin *> plugins = mSearchManager.searchPlugins();
    for (AbstractSearchPlugin *plugin : plugins) {
        const QSet<qint64> result = plugin->search(mQuery, mCollections, mMimeTypes);
        emitResults(result);
    }
}

void SearchRequest::exec()
{
    qCInfo(AKONADISERVER_SEARCH_LOG) << "Executing search" << mConnectionId;

    // TODO should we move this to the AgentSearchManager as well? If we keep it here the agents can be searched in parallel
    // since the plugin search is executed in this thread directly.
    searchPlugins();

    // If remote search is disabled, just finish here after searching the plugins
    if (!mRemoteSearch) {
        qCInfo(AKONADISERVER_SEARCH_LOG) << "Search " << mConnectionId << "done (without remote search)";
        return;
    }

    SearchTask task;
    task.id = mConnectionId;
    task.query = mQuery;
    task.mimeTypes = mMimeTypes;
    task.collections = mCollections;
    task.complete = false;

    mAgentSearchManager.addTask(&task);

    task.sharedLock.lock();
    for (;;) {
        if (task.complete) {
            break;
        }

        task.notifier.wait(&task.sharedLock);

        qCDebug(AKONADISERVER_SEARCH_LOG) << task.pendingResults.count() << "search results available in search" << task.id;
        if (!task.pendingResults.isEmpty()) {
            emitResults(task.pendingResults);
        }
        task.pendingResults.clear();
    }

    if (!task.pendingResults.isEmpty()) {
        emitResults(task.pendingResults);
    }
    task.sharedLock.unlock();

    qCInfo(AKONADISERVER_SEARCH_LOG) << "Search" << mConnectionId << "done (with remote search)";
}
