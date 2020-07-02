/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
