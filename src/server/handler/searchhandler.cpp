/***************************************************************************
 *   SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "searchhandler.h"

#include "akonadi.h"
#include "akonadiserver_search_debug.h"
#include "connection.h"
#include "handlerhelper.h"
#include "itemfetchhelper.h"
#include "search/agentsearchengine.h"
#include "search/searchmanager.h"
#include "search/searchrequest.h"
#include "searchhelper.h"

using namespace Akonadi;
using namespace Akonadi::Server;

SearchHandler::SearchHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool SearchHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::SearchCommand>(m_command);

    if (cmd.query().isEmpty()) {
        return failureResponse("No query specified");
    }

    QList<qint64> collectionIds;
    bool recursive = cmd.recursive();

    if (cmd.collections().isEmpty() || cmd.collections() == QList<qint64>{0LL}) {
        collectionIds << 0;
        recursive = true;
    }

    QList<qint64> collections = collectionIds;
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

    mItemFetchScope = cmd.itemFetchScope();
    mTagFetchScope = cmd.tagFetchScope();

    SearchRequest request(connection()->sessionId(), akonadi().searchManager(), akonadi().agentSearchManager());
    request.setCollections(collections);
    request.setMimeTypes(cmd.mimeTypes());
    request.setQuery(cmd.query());
    request.setRemoteSearch(cmd.remote());
    QObject::connect(&request, &SearchRequest::resultsAvailable, [this](const QSet<qint64> &results) {
        processResults(results);
    });
    request.exec();

    // qCDebug(AKONADISERVER_SEARCH_LOG) << "\tResult:" << uids;
    qCDebug(AKONADISERVER_SEARCH_LOG) << "\tResult:" << mAllResults.count() << "matches";

    return successResponse<Protocol::SearchResponse>();
}

void SearchHandler::processResults(const QSet<qint64> &results)
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

    ItemFetchHelper fetchHelper(connection(), scope, mItemFetchScope, mTagFetchScope, akonadi());
    fetchHelper.fetchItems();
}
