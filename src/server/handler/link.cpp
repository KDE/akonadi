/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "link.h"

#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/transaction.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionqueryhelper.h"
#include "entities.h"
#include "imapstreamparser.h"

#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool Link::parseStream()
{
    Protocol::LinkItemsCommand cmd;
    mInStream >> cmd;

    const Collection collection = HandlerHelper::collectionFromScope(cmd.destination());
    if (!collection.isVirtual()) {
        return failureResponse<Protocol::LinkItemsResponse>(
            QStringLiteral("Can't link items to non-virtual collections"));
    }

    /* FIXME BIN
    Resource originalContext;
    Scope::SelectionScope itemSelectionScope = Scope::selectionScopeFromByteArray(m_streamParser->peekString());
    if (itemSelectionScope != Scope::None) {
        m_streamParser->readString();
        // Unset Resource context if destination collection is specified using HRID/RID,
        // because otherwise the Resource context is relative to the destination collection
        // instead of the source collection (collection context)
        if ((mDestinationScope.scope() == Scope::HierarchicalRid || mDestinationScope.scope() == Scope::Rid) && itemSelectionScope == Scope::Rid) {
            originalContext = connection()->context()->resource();
            connection()->context()->setResource(Resource());
        }
    }
    Scope itemScope(itemSelectionScope);
    itemScope.parseScope(m_streamParser);
    */

    SelectQueryBuilder<PimItem> qb;
    ItemQueryHelper::scopeToQuery(cmd.scope(), connection()->context(), qb);

    /*
    if (originalContext.isValid()) {
        connection()->context()->setResource(originalContext);
    }
    */

    if (!qb.exec()) {
        return failureResponse<Protocol::LinkItemsResponse>(
            QStringLiteral("Unable to execute item query"));
    }

    const PimItem::List items = qb.result();

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store);

    PimItem::List toLink, toUnlink;
    const bool createLinks = (cmd.action() == Protocol::LinkItemsCommand::Link);
    for (const PimItem &item : items) {
        const bool alreadyLinked = collection.relatesToPimItem(item);
        bool result = true;
        if (createLinks && !alreadyLinked) {
            result = collection.addPimItem(item);
            toLink << item;
        } else if (!createLinks && alreadyLinked) {
            result = collection.removePimItem(item);
            toUnlink << item;
        }
        if (!result) {
            return failureResponse<Protocol::LinkItemsResponse>(
                QStringLiteral("Failed to modify item reference"));
        }
    }

    if (!toLink.isEmpty()) {
        store->notificationCollector()->itemsLinked(toLink, collection);
    } else if (!toUnlink.isEmpty()) {
        store->notificationCollector()->itemsUnlinked(toUnlink, collection);
    }

    if (!transaction.commit()) {
        return failureResponse<Protocol::LinkItemsResponse>(
            QStringLiteral("Cannot commit transaction."));
    }

    mOutStream << Protocol::LinkItemsResponse();
    return true;
}
