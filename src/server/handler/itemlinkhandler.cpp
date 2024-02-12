/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemlinkhandler.h"

#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

#include "private/scope_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

ItemLinkHandler::ItemLinkHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool ItemLinkHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::LinkItemsCommand>(m_command);

    const Collection collection = HandlerHelper::collectionFromScope(cmd.destination(), connection()->context());
    if (!collection.isVirtual()) {
        return failureResponse(QStringLiteral("Can't link items to non-virtual collections"));
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
    ItemQueryHelper::scopeToQuery(cmd.items(), connection()->context(), qb);

    /*
    if (originalContext.isValid()) {
        connection()->context()->setResource(originalContext);
    }
    */

    if (!qb.exec()) {
        return failureResponse(QStringLiteral("Unable to execute item query"));
    }

    const PimItem::List items = qb.result();
    const bool createLinks = (cmd.action() == Protocol::LinkItemsCommand::Link);

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store, createLinks ? QStringLiteral("LINK") : QStringLiteral("UNLINK"));

    PimItem::List toLink;
    PimItem::List toUnlink;
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
            return failureResponse(QStringLiteral("Failed to modify item reference"));
        }
    }

    if (!transaction.commit()) {
        return failureResponse(QStringLiteral("Cannot commit transaction."));
    }

    if (!toLink.isEmpty()) {
        store->notificationCollector()->itemsLinked(toLink, collection);
    } else if (!toUnlink.isEmpty()) {
        store->notificationCollector()->itemsUnlinked(toUnlink, collection);
    }

    return successResponse<Protocol::LinkItemsResponse>();
}
