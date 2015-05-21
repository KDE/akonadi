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
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "connection.h"
#include "storage/itemretriever.h"
#include "imapstreamparser.h"
#include "storage/transaction.h"
#include "storage/collectionqueryhelper.h"
#include "storage/selectquerybuilder.h"
#include "cachecleaner.h"

#include <shared/akdebug.h>
#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool ColMove::parseStream()
{
    Protocol::MoveCollectionCommand cmd;
    mInStream >> cmd;

    Collection source = HandlerHelper::collectionFromScope(cmd.collection());
    Collection target = HandlerHelper::collectionFromScope(cmd.destination());

    if (source.parentId() == target.id()) {
        mOutStream << Protocol::MoveCollectionResponse();
        return true;
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
        return failureResponse<Protocol::MoveCollectionResponse>(QStringLiteral("Unable to reparent collection"));
    }

    if (!transaction.commit()) {
        return failureResponse<Protocol::MoveCollectionResponse>(QStringLiteral("Cannot commit transaction."));
    }

    mOutStream << Protocol::MoveCollectionResponse();
    return true;
}
