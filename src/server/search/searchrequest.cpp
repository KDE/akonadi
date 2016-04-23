/*
    Copyright (c) 2013, 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "searchrequest.h"

#include <QPluginLoader>

#include "searchtaskmanager.h"
#include "abstractsearchplugin.h"
#include "searchmanager.h"
#include "connection.h"
#include "akonadiserver_debug.h"

#include <private/xdgbasedirs_p.h>

using namespace Akonadi::Server;

SearchRequest::SearchRequest(const QByteArray &connectionId)
    : mConnectionId(connectionId)
    , mRemoteSearch(true)
    , mStoreResults(false)
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

void SearchRequest::setCollections(const QVector<qint64> &collectionsIds)
{
    mCollections = collectionsIds;
}

QVector<qint64> SearchRequest::collections() const
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
    const QVector<AbstractSearchPlugin *> plugins = SearchManager::instance()->searchPlugins();
    Q_FOREACH (AbstractSearchPlugin *plugin, plugins) {
        const QSet<qint64> result = plugin->search(mQuery, mCollections.toList(), mMimeTypes);
        emitResults(result);
    }
}

void SearchRequest::exec()
{
    qCDebug(AKONADISERVER_LOG) << "Executing search" << mConnectionId;

    //TODO should we move this to the AgentSearchManager as well? If we keep it here the agents can be searched in parallel
    //since the plugin search is executed in this thread directly.
    searchPlugins();

    // If remote search is disabled, just finish here after searching the plugins
    if (!mRemoteSearch) {
        qCDebug(AKONADISERVER_LOG) << "Search done" << mConnectionId << "(without remote search)";
        return;
    }

    SearchTask task;
    task.id = mConnectionId;
    task.query = mQuery;
    task.mimeTypes = mMimeTypes;
    task.collections = mCollections;
    task.complete = false;

    SearchTaskManager::instance()->addTask(&task);

    task.sharedLock.lock();
    Q_FOREVER {
        if (task.complete) {
            qCDebug(AKONADISERVER_LOG) << "All queries processed!";
            break;
        } else {
            task.notifier.wait(&task.sharedLock);

            qCDebug(AKONADISERVER_LOG) << task.pendingResults.count() << "search results available in search" << task.id;
            if (!task.pendingResults.isEmpty()) {
                emitResults(task.pendingResults);
            }
            task.pendingResults.clear();
        }
    }

    if (!task.pendingResults.isEmpty()) {
        emitResults(task.pendingResults);
    }
    task.sharedLock.unlock();

    qCDebug(AKONADISERVER_LOG) << "Search done" << mConnectionId;
}
