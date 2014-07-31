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

#include <connection.h>
#include <entities.h>
#include <imapstreamparser.h>
#include <storage/datastore.h>
#include <storage/itemqueryhelper.h>
#include <storage/selectquerybuilder.h>
#include <storage/transaction.h>
#include <libs/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

Remove::Remove(Scope::SelectionScope scope)
    : mScope(scope)
{
}

bool Remove::parseStream()
{
    mScope.parseScope(m_streamParser);
    connection()->context()->parseContext(m_streamParser);
    qDebug() << "Tag context:" << connection()->context()->tagId();
    qDebug() << "Collection context: " << connection()->context()->collectionId();

    SelectQueryBuilder<PimItem> qb;
    ItemQueryHelper::scopeToQuery(mScope, connection()->context(), qb);

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store);

    if (qb.exec()) {
        const QVector<PimItem> items = qb.result();
        if (items.isEmpty()) {
            throw HandlerException("No items found");
        }
        if (!store->cleanupPimItems(items)) {
            throw HandlerException("Deletion failed");
        }
    } else {
        throw HandlerException("Unable to execute query");
    }

    if (!transaction.commit()) {
        return failureResponse("Unable to commit transaction.");
    }

    return successResponse("REMOVE complete");
}
