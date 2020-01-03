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

#include "collectionmovehandler.h"

#include "akonadi.h"
#include "handlerhelper.h"
#include "connection.h"
#include "cachecleaner.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"
#include "storage/transaction.h"
#include "storage/collectionqueryhelper.h"

using namespace Akonadi;
using namespace Akonadi::Server;

CollectionMoveHandler::CollectionMoveHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

bool CollectionMoveHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::MoveCollectionCommand>(m_command);

    Collection source = HandlerHelper::collectionFromScope(cmd.collection(), connection()->context());
    if (!source.isValid()) {
        return failureResponse(QStringLiteral("Invalid collection to move"));
    }

    Collection target;
    if (cmd.destination().isEmpty()) {
        target.setId(0);
    } else {
        target = HandlerHelper::collectionFromScope(cmd.destination(), connection()->context());
        if (!target.isValid()) {
            return failureResponse(QStringLiteral("Invalid destination collection"));
        }
    }

    if (source.parentId() == target.id()) {
        return successResponse<Protocol::MoveCollectionResponse>();
    }

    CacheCleanerInhibitor inhibitor(akonadi());

    // retrieve all not yet cached items of the source
    ItemRetriever retriever(akonadi().itemRetrievalManager(), connection(), connection()->context());
    retriever.setCollection(source, true);
    retriever.setRetrieveFullPayload(true);
    if (!retriever.exec()) {
        return failureResponse(retriever.lastError());
    }

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store, QStringLiteral("CollectionMoveHandler"));

    if (!store->moveCollection(source, target)) {
        return failureResponse(QStringLiteral("Unable to reparent collection"));
    }

    if (!transaction.commit()) {
        return failureResponse(QStringLiteral("Cannot commit transaction."));
    }

    return successResponse<Protocol::MoveCollectionResponse>();
}
