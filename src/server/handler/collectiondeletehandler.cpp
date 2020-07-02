/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectiondeletehandler.h"

#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionqueryhelper.h"
#include "search/searchmanager.h"

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

CollectionDeleteHandler::CollectionDeleteHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

bool CollectionDeleteHandler::deleteRecursive(Collection &col)
{
    Collection::List children = col.children();
    for (Collection &child : children) {
        if (!deleteRecursive(child)) {
            return false;
        }
    }

    DataStore *db = connection()->storageBackend();
    return db->cleanupCollection(col);
}

bool CollectionDeleteHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::DeleteCollectionCommand>(m_command);

    Collection collection = HandlerHelper::collectionFromScope(cmd.collection(), connection()->context());
    if (!collection.isValid()) {
        return failureResponse(QStringLiteral("No such collection."));
    }

    // handle virtual folders
    if (collection.resource().name() == QLatin1String(AKONADI_SEARCH_RESOURCE)) {
        // don't delete virtual root
        if (collection.parentId() == 0) {
            return failureResponse(QStringLiteral("Cannot delete virtual root collection"));
        }
    }

    Transaction transaction(storageBackend(), QStringLiteral("DELETE"));

    if (!deleteRecursive(collection)) {
        return failureResponse(QStringLiteral("Unable to delete collection"));
    }

    if (!transaction.commit()) {
        return failureResponse(QStringLiteral("Unable to commit transaction"));
    }

    return successResponse<Protocol::DeleteCollectionResponse>();
}
