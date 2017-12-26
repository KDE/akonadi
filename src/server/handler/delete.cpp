/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "delete.h"

#include "akonadi.h"
#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionqueryhelper.h"
#include "search/searchmanager.h"
#include "indexer/indexer.h"
#include "indexer/indexfuture.h"

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool Delete::deleteRecursive(Collection &col, IndexFutureSet &futures)
{
    Collection::List children = col.children();
    for (Collection &child : children) {
        if (!deleteRecursive(child, futures)) {
            return false;
        }
    }

    const auto mimeTypes = col.mimeTypes();
    QStringList mts;
    mts.reserve(mimeTypes.count());
    for (const auto &mt : mimeTypes) {
        mts.push_back(mt.name());
    }
    auto indexer = AkonadiServer::instance()->indexer();
    futures.add(indexer->removeCollection(col.id(), mts));

    DataStore *db = connection()->storageBackend();
    return db->cleanupCollection(col);
}

bool Delete::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::DeleteCollectionCommand>(m_command);

    Collection collection = HandlerHelper::collectionFromScope(cmd.collection(), connection());
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

    Transaction transaction(DataStore::self(), QStringLiteral("DELETE"));

    IndexFutureSet futures;

    if (!deleteRecursive(collection, futures)) {
        return failureResponse(QStringLiteral("Unable to delete collection"));
    }

    if (!transaction.commit()) {
        return failureResponse(QStringLiteral("Unable to commit transaction"));
    }

    futures.waitForAll();

    return successResponse<Protocol::DeleteCollectionResponse>();
}
