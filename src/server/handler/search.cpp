/***************************************************************************
 *   Copyright (C) 2009 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "search.h"

#include "connection.h"
#include "fetchhelper.h"
#include "handlerhelper.h"
#include "searchhelper.h"
#include "search/searchrequest.h"
#include "search/searchmanager.h"
#include "akonadiserver_search_debug.h"

using namespace Akonadi;
using namespace Akonadi::Server;

bool Search::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::SearchCommand>(m_command);

    if (cmd.query().isEmpty()) {
        return failureResponse("No query specified");
    }

    QVector<qint64> collectionIds;
    bool recursive = cmd.recursive();

    if (cmd.collections().isEmpty() || cmd.collections() == QVector<qint64> { 0ll }) {
        collectionIds << 0;
        recursive = true;
    }

    QVector<qint64> collections = collectionIds;
    if (recursive) {
        collections += SearchHelper::matchSubcollectionsByMimeType(collectionIds, cmd.mimeTypes());
    }

    qCDebug(AKONADISERVER_SEARCH_LOG) << "SEARCH:";
    qCDebug(AKONADISERVER_SEARCH_LOG) << "\tQuery:" << cmd.query();
    qCDebug(AKONADISERVER_SEARCH_LOG) << "\tMimeTypes:" << cmd.mimeTypes();
    qCDebug(AKONADISERVER_SEARCH_LOG) << "\tCollections:" << collections;
    qCDebug(AKONADISERVER_SEARCH_LOG) << "\tRemote:" << cmd.remote();
    qCDebug(AKONADISERVER_SEARCH_LOG) << "\tRecursive" << recursive;

    if (collections.isEmpty()) {
        return successResponse<Protocol::SearchResponse>();
    }

    mFetchScope = cmd.fetchScope();

    SearchRequest request(connection()->sessionId());
    request.setCollections(collections);
    request.setMimeTypes(cmd.mimeTypes());
    request.setQuery(cmd.query());
    request.setRemoteSearch(cmd.remote());
    QObject::connect(&request, &SearchRequest::resultsAvailable,
                     [this](const QSet<qint64> &results) {
                        processResults(results);
                     });
    request.exec();

    //qCDebug(AKONADISERVER_SEARCH_LOG) << "\tResult:" << uids;
    qCDebug(AKONADISERVER_SEARCH_LOG) << "\tResult:" << mAllResults.count() << "matches";

    return successResponse<Protocol::SearchResponse>();
}

void Search::processResults(const QSet<qint64> &results)
{
    QSet<qint64> newResults = results;
    newResults.subtract(mAllResults);
    mAllResults.unite(newResults);

    if (newResults.isEmpty()) {
        return;
    }

    ImapSet imapSet;
    imapSet.add(newResults);

    Scope scope;
    scope.setUidSet(imapSet);

    FetchHelper fetchHelper(connection(), scope, mFetchScope);
    fetchHelper.fetchItems();
}
