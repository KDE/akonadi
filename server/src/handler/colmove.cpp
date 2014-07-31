/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "colmove.h"
#include <handlerhelper.h>
#include <storage/datastore.h>
#include <connection.h>
#include <storage/itemretriever.h>
#include <imapstreamparser.h>
#include <storage/transaction.h>
#include <storage/collectionqueryhelper.h>
#include <storage/selectquerybuilder.h>
#include <cachecleaner.h>
#include <akdebug.h>

using namespace Akonadi;
using namespace Akonadi::Server;

ColMove::ColMove(Scope::SelectionScope scope)
    : m_scope(scope)
{
}

bool ColMove::parseStream()
{
    m_scope.parseScope(m_streamParser);
    SelectQueryBuilder<Collection> qb;
    CollectionQueryHelper::scopeToQuery(m_scope, connection(), qb);
    if (!qb.exec()) {
        throw HandlerException("Unable to execute collection query");
    }
    const Collection::List sources = qb.result();
    if (sources.isEmpty()) {
        throw HandlerException("No source collection specified");
    } else if (sources.size() > 1) {   // TODO
        throw HandlerException("Moving multiple collections is not yet implemented");
    }
    Collection source = sources.first();

    Scope destScope(m_scope.scope());
    destScope.parseScope(m_streamParser);
    akDebug() << destScope.uidSet().toImapSequenceSet();
    const Collection target = CollectionQueryHelper::singleCollectionFromScope(destScope, connection());

    if (source.parentId() == target.id()) {
        return successResponse("COLMOVE complete - nothing to do");
    }

    CacheCleanerInhibitor inhibitor;

    // retrieve all not yet cached items of the source
    ItemRetriever retriever(connection());
    retriever.setCollection(source, true);
    retriever.setRetrieveFullPayload(true);
    if (!retriever.exec()) {
        return failureResponse(retriever.lastError());
    }

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store);

    if (!store->moveCollection(source, target)) {
        return failureResponse("Unable to reparent collection");
    }

    if (!transaction.commit()) {
        return failureResponse("Cannot commit transaction.");
    }

    return successResponse("COLMOVE complete");
}
