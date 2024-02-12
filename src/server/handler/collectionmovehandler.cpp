/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionmovehandler.h"
#include "akonadiserver_debug.h"

#include "akonadi.h"
#include "cachecleaner.h"
#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"
#include "storage/transaction.h"

using namespace Akonadi;
using namespace Akonadi::Server;

namespace
{

bool isRootCollection(const Scope &scope)
{
    return scope.scope() == Scope::Uid && scope.uid() == 0;
}

}

CollectionMoveHandler::CollectionMoveHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool CollectionMoveHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::MoveCollectionCommand>(m_command);

    Collection source = HandlerHelper::collectionFromScope(cmd.collection(), connection()->context());
    if (!source.isValid()) {
        return failureResponse(QStringLiteral("Invalid collection to move"));
    }

    Collection target;
    if (!checkScopeConstraints(cmd.destination(), {Scope::Uid, Scope::HierarchicalRid})) {
        qCWarning(AKONADISERVER_LOG) << "Only UID and HRID-based move destination are supported.";
        return failureResponse("Only UID and HRID-based move destination are supported");
    }
    if (isRootCollection(cmd.destination())) {
        target.setId(0);
    } else {
        target = HandlerHelper::collectionFromScope(cmd.destination(), connection()->context());
        if (!target.isValid()) {
            qCWarning(AKONADISERVER_LOG) << "Invalid destination collection" << cmd.destination();
            return failureResponse("Invalid destination collection");
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
