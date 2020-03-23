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

#include "itemmovehandler.h"

#include "akonadi.h"
#include "connection.h"
#include "handlerhelper.h"
#include "cachecleaner.h"
#include "storage/datastore.h"
#include "storage/itemretrievalmanager.h"
#include "storage/itemretriever.h"
#include "storage/itemqueryhelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"
#include "storage/collectionqueryhelper.h"
#include "akonadiserver_debug.h"

using namespace Akonadi;
using namespace Akonadi::Server;

ItemMoveHandler::ItemMoveHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

void ItemMoveHandler::itemsRetrieved(const QVector<qint64> &ids)
{
    DataStore *store = connection()->storageBackend();
    Transaction transaction(store, QStringLiteral("MOVE"));

    SelectQueryBuilder<PimItem> qb;
    qb.setForUpdate();
    ItemQueryHelper::itemSetToQuery(ImapSet(ids), qb);
    qb.addValueCondition(PimItem::collectionIdFullColumnName(), Query::NotEquals, mDestination.id());


    if (!qb.exec()) {
        failureResponse("Unable to execute query");
        return;
    }

    const QVector<PimItem> items = qb.result();
    if (items.isEmpty()) {
        return;
    }

    const QDateTime mtime = QDateTime::currentDateTimeUtc();
    // Split the list by source collection
    QMap<Entity::Id /* collection */, PimItem> toMove;
    QMap<Entity::Id /* collection */, Collection> sources;
    ImapSet toMoveIds;
    Q_FOREACH (/*sic!*/ PimItem item, items) {  //krazy:exclude=foreach
        if (!item.isValid()) {
            failureResponse("Invalid item in result set!?");
            return;
        }

        const Collection source = item.collection();
        if (!source.isValid()) {
            failureResponse("Item without collection found!?");
            return;
        }
        if (!sources.contains(source.id())) {
            sources.insert(source.id(), source);
        }

        Q_ASSERT(item.collectionId() != mDestination.id());

        item.setCollectionId(mDestination.id());
        item.setAtime(mtime);
        item.setDatetime(mtime);
        // if the resource moved itself, we assume it did so because the change happenned in the backend
        if (connection()->context().resource().id() != mDestination.resourceId()) {
            item.setDirty(true);
        }

        if (!item.update()) {
            failureResponse("Unable to update item");
            return;
        }

        toMove.insertMulti(source.id(), item);
        toMoveIds.add(QVector<qint64>{ item.id() });
    }

    if (!transaction.commit()) {
        failureResponse("Unable to commit transaction.");
        return;
    }

    // Emit notification for each source collection separately
    Collection source;
    PimItem::List itemsToMove;
    for (auto it = toMove.cbegin(), end = toMove.cend(); it != end; ++it) {
        if (source.id() != it.key()) {
            if (!itemsToMove.isEmpty()) {
                store->notificationCollector()->itemsMoved(itemsToMove, source, mDestination);
            }
            source = sources.value(it.key());
            itemsToMove.clear();
        }

        itemsToMove.push_back(*it);
    }

    if (!itemsToMove.isEmpty()) {
        store->notificationCollector()->itemsMoved(itemsToMove, source, mDestination);
    }

    // Batch-reset RID
    // The item should have an empty RID in the destination collection to avoid
    // RID conflicts with existing items (see T3904 in Phab).
    // We do it after emitting notification so that the FetchHelper can still
    // retrieve the RID
    QueryBuilder qb2(PimItem::tableName(), QueryBuilder::Update);
    qb2.setColumnValue(PimItem::remoteIdColumn(), QString());
    ItemQueryHelper::itemSetToQuery(toMoveIds, connection()->context(), qb2);
    if (!qb2.exec()) {
        failureResponse("Unable to update RID");
        return;
    }

}

bool ItemMoveHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::MoveItemsCommand>(m_command);

    mDestination = HandlerHelper::collectionFromScope(cmd.destination(), connection()->context());
    if (mDestination.isVirtual()) {
        return failureResponse("Moving items into virtual collection is not allowed");
    }
    if (!mDestination.isValid()) {
        return failureResponse("Invalid destination collection");
    }

    CommandContext context = connection()->context();
    context.setScopeContext(cmd.itemsContext());
    if (cmd.items().scope() == Scope::Rid) {
        if (!context.collection().isValid()) {
            return failureResponse("RID move requires valid source collection");
        }
    }

    CacheCleanerInhibitor inhibitor(akonadi());

    // make sure all the items we want to move are in the cache
    ItemRetriever retriever(akonadi().itemRetrievalManager(), connection(), context);
    retriever.setScope(cmd.items());
    retriever.setRetrieveFullPayload(true);
    QObject::connect(&retriever, &ItemRetriever::itemsRetrieved,
                     [this](const QVector<qint64> &ids) {
                        itemsRetrieved(ids);
                     });
    if (!retriever.exec()) {
        return failureResponse(retriever.lastError());
    }

    return successResponse<Protocol::MoveItemsResponse>();
}
