/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemdeletehandler.h"

#include "connection.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

ItemDeleteHandler::ItemDeleteHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool ItemDeleteHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::DeleteItemsCommand>(m_command);

    CommandContext context = connection()->context();
    if (!context.setScopeContext(cmd.scopeContext())) {
        return failureResponse(QStringLiteral("Invalid scope context"));
    }

    SelectQueryBuilder<PimItem> qb;
    ItemQueryHelper::scopeToQuery(cmd.items(), context, qb);

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store, QStringLiteral("REMOVE"));

    if (!qb.exec()) {
        return failureResponse("Unable to execute query");
    }

    const QList<PimItem> items = qb.result();
    if (items.isEmpty()) {
        return failureResponse("No items found");
    }
    if (!store->cleanupPimItems(items)) {
        return failureResponse("Deletion failed");
    }

    if (!transaction.commit()) {
        return failureResponse("Unable to commit transaction");
    }

    return successResponse<Protocol::DeleteItemsResponse>();
}
