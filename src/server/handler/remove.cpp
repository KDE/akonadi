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

#include "remove.h"

#include "connection.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool Remove::parseStream()
{
    Protocol::DeleteItemsCommand cmd(m_command);

    connection()->context()->setScopeContext(cmd.scopeContext());

    SelectQueryBuilder<PimItem> qb;
    ItemQueryHelper::scopeToQuery(cmd.items(), connection()->context(), qb);

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store, QStringLiteral("REMOVE"));

    if (!qb.exec()) {
        return failureResponse("Unable to execute query");
    }

    const QVector<PimItem> items = qb.result();
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
