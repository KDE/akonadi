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

#include "move.h"

#include "connection.h"
#include "handlerhelper.h"
#include "cachecleaner.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"
#include "storage/itemqueryhelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"
#include "storage/collectionqueryhelper.h"

using namespace Akonadi;
using namespace Akonadi::Server;

void Move::itemsRetrieved(const QList<qint64> &ids)
{

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store);

    SelectQueryBuilder<PimItem> qb;
    ItemQueryHelper::itemSetToQuery(ImapSet(ids), qb);
    qb.addValueCondition(PimItem::collectionIdFullColumnName(), Query::NotEquals, mDestination.id());

    const QDateTime mtime = QDateTime::currentDateTime();

    if (qb.exec()) {
        const QVector<PimItem> items = qb.result();
        if (items.isEmpty()) {
            successResponse<Protocol::MoveItemsResponse>();
            return;
        }

        // Split the list by source collection
        QMap<Entity::Id /* collection */, PimItem> toMove;
        QMap<Entity::Id /* collection */, Collection> sources;
        Q_FOREACH (/*sic!*/ PimItem item, items) {
            const Collection source = items.at(0).collection();
            if (!source.isValid()) {
                failureResponse("Item without collection found!?");
                return;
            }
            if (!sources.contains(source.id())) {
                sources.insert(source.id(), source);
            }

            if (!item.isValid()) {
                failureResponse("Invalid item in result set!?");
                return;
            }
            Q_ASSERT(item.collectionId() != mDestination.id());

            item.setCollectionId(mDestination.id());
            item.setAtime(mtime);
            item.setDatetime(mtime);
            // if the resource moved itself, we assume it did so because the change happend in the backend
            if (connection()->context()->resource().id() != mDestination.resourceId()) {
                item.setDirty(true);
            }

            toMove.insertMulti(source.id(), item);
        }

        // Emit notification for each source collection separately
        Q_FOREACH (const Entity::Id &sourceId, toMove.uniqueKeys()) {
            PimItem::List itemsToMove;
            for (auto it = toMove.cbegin(), end = toMove.cend(); it != end; ++it) {
                if (it.key() == sourceId)
                    itemsToMove.push_back(it.value());
            }

            const Collection &source = sources.value(sourceId);
            store->notificationCollector()->itemsMoved(itemsToMove, source, mDestination);

            for (auto iter = toMove.find(sourceId), end = toMove.end(); iter != end; ++iter) {
                // reset RID on inter-resource moves, but only after generating the change notification
                // so that this still contains the old one for the source resource
                const bool isInterResourceMove = (*iter).collection().resource().id() != source.resource().id();
                if (isInterResourceMove) {
                    (*iter).setRemoteId(QString());
                }

                // FIXME Could we aggregate the changes to a single SQL query?
                if (!(*iter).update()) {
                    failureResponse("Unable to update item");
                    return;
                }
            }
        }
    } else {
        failureResponse("Unable to execute query");
        return;
    }

    if (!transaction.commit()) {
        failureResponse("Unable to commit transaction.");
        return;
    }

}

bool Move::parseStream()
{
    Protocol::MoveItemsCommand cmd(m_command);

    mDestination = HandlerHelper::collectionFromScope(cmd.destination(), connection());
    if (mDestination.isVirtual()) {
        return failureResponse("Moving items into virtual collection is not allowed");
    }
    if (!mDestination.isValid()) {
        return failureResponse("Invalid destination collection");
    }

    connection()->context()->setScopeContext(cmd.itemsContext());
    if (cmd.items().scope() == Scope::Rid) {
        if (!connection()->context()->collection().isValid()) {
            return failureResponse("RID move requires valid source collection");
        }
    }

    CacheCleanerInhibitor inhibitor;

    // make sure all the items we want to move are in the cache
    ItemRetriever retriever(connection());
    retriever.setScope(cmd.items());
    retriever.setRetrieveFullPayload(true);
    connect(&retriever, &ItemRetriever::itemsRetrieved,
            this, &Move::itemsRetrieved);
    if (!retriever.exec()) {
        return failureResponse(retriever.lastError());
    }

    return successResponse<Protocol::MoveItemsResponse>();
}
