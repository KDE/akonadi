/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Milian Wolff <mail@milianw.de>

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

#include "itemretriever.h"

#include "akonadi.h"
#include "connection.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretrievalmanager.h"
#include "storage/itemretrievalrequest.h"
#include "storage/parthelper.h"
#include "storage/parttypehelper.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"
#include "utils.h"

#include <shared/akranges.h>
#include <private/protocol_p.h>

#include <QEventLoop>

#include "akonadiserver_debug.h"

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

Q_DECLARE_METATYPE(ItemRetrievalResult)

ItemRetriever::ItemRetriever(ItemRetrievalManager &manager, Connection *connection, const CommandContext &context)
    : mScope()
    , mItemRetrievalManager(manager)
    , mConnection(connection)
    , mContext(context)
    , mFullPayload(false)
    , mRecursive(false)
    , mCanceled(false)
{
    qRegisterMetaType<ItemRetrievalResult>("Akonadi::Server::ItemRetrievalResult");
    if (mConnection) {
        connect(mConnection, &Connection::disconnected,
                this, [this]() {
                    mCanceled = true;
                });
    }
}

Connection *ItemRetriever::connection() const
{
    return mConnection;
}

void ItemRetriever::setRetrieveParts(const QVector<QByteArray> &parts)
{
    mParts = parts;
    std::sort(mParts.begin(), mParts.end());
    mParts.erase(std::unique(mParts.begin(), mParts.end()), mParts.end());

    // HACK, we need a full payload available flag in PimItem
    if (mFullPayload && !mParts.contains(AKONADI_PARAM_PLD_RFC822)) {
        mParts.append(AKONADI_PARAM_PLD_RFC822);
    }
}

void ItemRetriever::setItemSet(const ImapSet &set, const Collection &collection)
{
    mItemSet = set;
    mCollection = collection;
}

void ItemRetriever::setItemSet(const ImapSet &set, bool isUid)
{
    if (!isUid && mContext.collectionId() >= 0) {
        setItemSet(set, mContext.collection());
    } else {
        setItemSet(set);
    }
}

void ItemRetriever::setItem(const Entity::Id &id)
{
    ImapSet set;
    set.add(ImapInterval(id, id));
    mItemSet = set;
    mCollection = Collection();
}

void ItemRetriever::setRetrieveFullPayload(bool fullPayload)
{
    mFullPayload = fullPayload;
    // HACK, we need a full payload available flag in PimItem
    if (fullPayload && !mParts.contains(AKONADI_PARAM_PLD_RFC822)) {
        mParts.append(AKONADI_PARAM_PLD_RFC822);
    }
}

void ItemRetriever::setCollection(const Collection &collection, bool recursive)
{
    mCollection = collection;
    mItemSet = ImapSet();
    mRecursive = recursive;
}

void ItemRetriever::setScope(const Scope &scope)
{
    mScope = scope;
}

Scope ItemRetriever::scope() const
{
    return mScope;
}

void ItemRetriever::setChangedSince(const QDateTime &changedSince)
{
    mChangedSince = changedSince;
}

QVector<QByteArray> ItemRetriever::retrieveParts() const
{
    return mParts;
}

enum QueryColumns {
    PimItemIdColumn,

    CollectionIdColumn,
    ResourceIdColumn,

    PartTypeNameColumn,
    PartDatasizeColumn
};

QSqlQuery ItemRetriever::buildQuery() const
{
    QueryBuilder qb(PimItem::tableName());

    qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());

    qb.addJoin(QueryBuilder::LeftJoin, Part::tableName(), PimItem::idFullColumnName(), Part::pimItemIdFullColumnName());

    Query::Condition partTypeJoinCondition;
    partTypeJoinCondition.addColumnCondition(Part::partTypeIdFullColumnName(), Query::Equals, PartType::idFullColumnName());
    if (!mFullPayload && !mParts.isEmpty()) {
        partTypeJoinCondition.addCondition(PartTypeHelper::conditionFromFqNames(mParts));
    }
    partTypeJoinCondition.addValueCondition(PartType::nsFullColumnName(), Query::Equals, QStringLiteral("PLD"));
    qb.addJoin(QueryBuilder::LeftJoin, PartType::tableName(), partTypeJoinCondition);

    qb.addColumn(PimItem::idFullColumnName());
    qb.addColumn(PimItem::collectionIdFullColumnName());
    qb.addColumn(Collection::resourceIdFullColumnName());
    qb.addColumn(PartType::nameFullColumnName());
    qb.addColumn(Part::datasizeFullColumnName());

    if (!mItemSet.isEmpty() || mCollection.isValid()) {
        ItemQueryHelper::itemSetToQuery(mItemSet, qb, mCollection);
    } else {
        ItemQueryHelper::scopeToQuery(mScope, mContext, qb);
    }

    // prevent a resource to trigger item retrieval from itself
    if (mConnection) {
        const Resource res = Resource::retrieveByName(QString::fromUtf8(mConnection->sessionId()));
        if (res.isValid()) {
            qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::NotEquals, res.id());
        }
    }

    if (mChangedSince.isValid()) {
        qb.addValueCondition(PimItem::datetimeFullColumnName(), Query::GreaterOrEqual,
                             mChangedSince.toUTC());
    }

    qb.addSortColumn(PimItem::idFullColumnName(), Query::Ascending);

    if (!qb.exec()) {
        mLastError = "Unable to retrieve items";
        throw ItemRetrieverException(mLastError);
    }

    qb.query().next();

    return qb.query();
}

namespace
{
static bool hasAllParts(const ItemRetrievalRequest &req, const QSet<QByteArray> &availableParts)
{
    return std::all_of(req.parts.begin(), req.parts.end(), [&availableParts](const auto &part) {
        return availableParts.contains(part);
    });
}
}

bool ItemRetriever::runItemRetrievalRequests(std::list<ItemRetrievalRequest> requests)
{
    QEventLoop eventLoop;
    std::vector<ItemRetrievalRequest::Id> pendingRequests;
    connect(&mItemRetrievalManager, &ItemRetrievalManager::requestFinished,
            this, [this, &eventLoop, &pendingRequests](const ItemRetrievalResult &result) {
                    const auto requestId = std::find(pendingRequests.begin(), pendingRequests.end(), result.request.id);
                    if (requestId != pendingRequests.end()) {
                        if (mCanceled) {
                            eventLoop.exit(1);
                        } else if (result.errorMsg.has_value()) {
                            mLastError = result.errorMsg->toUtf8();
                            eventLoop.exit(1);
                        } else {
                            Q_EMIT itemsRetrieved(result.request.ids);
                            pendingRequests.erase(requestId);
                            if (pendingRequests.empty()) {
                                eventLoop.quit();
                            }
                        }
                    }
    }, Qt::UniqueConnection);

    if (mConnection) {
        connect(mConnection, &Connection::connectionClosing,
                &eventLoop, [&eventLoop]() { eventLoop.exit(1); });
    }

    for (auto &&request : requests) {
        if ((!mFullPayload && request.parts.isEmpty()) || request.ids.isEmpty()) {
            continue;
        }

        // TODO: how should we handle retrieval errors here? so far they have been ignored,
        // which makes sense in some cases, do we need a command parameter for this?
        try {
            // Request is deleted inside ItemRetrievalManager, so we need to take
            // a copy here
            //const auto ids = request->ids;
            pendingRequests.push_back(request.id);
            mItemRetrievalManager.requestItemDelivery(std::move(request));
        } catch (const ItemRetrieverException &e) {
            qCCritical(AKONADISERVER_LOG) << e.type() << ": " << e.what();
            mLastError = e.what();
            return false;
        }
    }

    if (!pendingRequests.empty()) {
        if (eventLoop.exec()) {
            return false;
        }
    }

    return true;
}

akOptional<ItemRetriever::PreparedRequests> ItemRetriever::prepareRequests(QSqlQuery &query, const QByteArrayList &parts)
{
    QHash<qint64, QString> resourceIdNameCache;
    std::list<ItemRetrievalRequest> requests;
    QHash<qint64 /* collection */, decltype(requests)::iterator> colRequests;
    QHash<qint64 /* item */, decltype(requests)::iterator> itemRequests;
    QVector<qint64> readyItems;
    qint64 prevPimItemId = -1;
    QSet<QByteArray> availableParts;
    decltype(requests)::iterator lastRequest = requests.end();
    while (query.isValid()) {
        const qint64 pimItemId = query.value(PimItemIdColumn).toLongLong();
        const qint64 collectionId = query.value(CollectionIdColumn).toLongLong();
        const qint64 resourceId = query.value(ResourceIdColumn).toLongLong();
        const auto itemIter = itemRequests.constFind(pimItemId);

        if (Q_UNLIKELY(mCanceled)) {
            return nullopt;
        }

        if (pimItemId == prevPimItemId) {
            if (query.value(PartTypeNameColumn).isNull()) {
                // This is not the first part of the Item we saw, but LEFT JOIN PartTable
                // returned a null row - that means the row is an ATR part
                // which we don't care about
                query.next();
                continue;
            }
        } else {
            if (lastRequest != requests.end()) {
                if (hasAllParts(*lastRequest, availableParts)) {
                    // We went through all parts of a single item, if we have all
                    // parts available in the DB and they are not expired, then
                    // exclude this item from the retrieval
                    lastRequest->ids.removeOne(prevPimItemId);
                    itemRequests.remove(prevPimItemId);
                    readyItems.push_back(prevPimItemId);
                }
            }
            availableParts.clear();
            prevPimItemId = pimItemId;
        }

        if (itemIter != itemRequests.constEnd()) {
            lastRequest = itemIter.value();
        } else {
            const auto colIt = colRequests.find(collectionId);
            lastRequest = (colIt == colRequests.end()) ? requests.end() : colIt.value();
            if (lastRequest == requests.end() || lastRequest->ids.size() > 100) {
                requests.emplace_front(ItemRetrievalRequest{});
                lastRequest = requests.begin();
                lastRequest->ids.push_back(pimItemId);
                auto resIter = resourceIdNameCache.find(resourceId);
                if (resIter == resourceIdNameCache.end()) {
                    resIter = resourceIdNameCache.insert(resourceId, Resource::retrieveById(resourceId).name());
                }
                lastRequest->resourceId = *resIter;
                lastRequest->parts = parts;
                colRequests.insert(collectionId, lastRequest);
                itemRequests.insert(pimItemId, lastRequest);
            } else {
                lastRequest->ids.push_back(pimItemId);
                itemRequests.insert(pimItemId, lastRequest);
                colRequests.insert(collectionId, lastRequest);
            }
        }
        Q_ASSERT(lastRequest != requests.end());

        if (query.value(PartTypeNameColumn).isNull()) {
            // LEFT JOIN did not find anything, retrieve all parts
            query.next();
            continue;
        }

        qint64 datasize = query.value(PartDatasizeColumn).toLongLong();
        const QByteArray partName = Utils::variantToByteArray(query.value(PartTypeNameColumn));
        Q_ASSERT(!partName.startsWith(AKONADI_PARAM_PLD));
        if (datasize <= 0) {
            // request update for this part
            if (mFullPayload && !lastRequest->parts.contains(partName)) {
                lastRequest->parts.push_back(partName);
            }
        } else {
            // add the part to list of available parts, we will compare it with
            // the list of request parts once we handle all parts of this item
            availableParts.insert(partName);
        }
        query.next();
    }
    query.finish();

    // Post-check in case we only queried one item thus did not reach the check
    // at the beginning of the while() loop above
    if (lastRequest != requests.end() && hasAllParts(*lastRequest, availableParts)) {
        lastRequest->ids.removeOne(prevPimItemId);
        readyItems.push_back(prevPimItemId);
        // No need to update the hashtable at this point
    }

    return PreparedRequests{std::move(requests), std::move(readyItems)};
}

bool ItemRetriever::exec()
{
    if (mParts.isEmpty() && !mFullPayload) {
        return true;
    }

    verifyCache();

    QSqlQuery query = buildQuery();
    const auto parts = mParts | Views::filter([](const auto &part) { return part.startsWith(AKONADI_PARAM_PLD); })
                              | Views::transform([](const auto &part) { return part.mid(4); })
                              | Actions::toQList;

    const auto requests = prepareRequests(query, parts);
    if (!requests.has_value()) {
        return false;
    }

    if (!requests->readyItems.isEmpty()) {
        Q_EMIT itemsRetrieved(requests->readyItems);
    }

    if (!runItemRetrievalRequests(std::move(requests->requests))) {
        return false;
    }

    // retrieve items in child collections if requested
    bool result = true;
    if (mRecursive && mCollection.isValid()) {
        Q_FOREACH (const Collection &col, mCollection.children()) {
            ItemRetriever retriever(mItemRetrievalManager, mConnection, mContext);
            retriever.setCollection(col, mRecursive);
            retriever.setRetrieveParts(mParts);
            retriever.setRetrieveFullPayload(mFullPayload);
            connect(&retriever, &ItemRetriever::itemsRetrieved,
                    this, &ItemRetriever::itemsRetrieved);
            result = retriever.exec();
            if (!result) {
                break;
            }
        }
    }

    return result;
}

void ItemRetriever::verifyCache()
{
    if (!connection() || !connection()->verifyCacheOnRetrieval()) {
        return;
    }

    SelectQueryBuilder<Part> qb;
    qb.addJoin(QueryBuilder::InnerJoin, PimItem::tableName(), Part::pimItemIdFullColumnName(), PimItem::idFullColumnName());
    qb.addValueCondition(Part::storageFullColumnName(), Query::Equals, Part::External);
    qb.addValueCondition(Part::dataFullColumnName(), Query::IsNot, QVariant());
    if (mScope.scope() != Scope::Invalid) {
        ItemQueryHelper::scopeToQuery(mScope, mContext, qb);
    } else {
        ItemQueryHelper::itemSetToQuery(mItemSet, qb, mCollection);
    }

    if (!qb.exec()) {
        mLastError = QByteArrayLiteral("Unable to query parts.");
        throw ItemRetrieverException(mLastError);
    }

    const Part::List externalParts = qb.result();
    for (Part part : externalParts) {
        PartHelper::verify(part);
    }
}

QByteArray ItemRetriever::lastError() const
{
    return mLastError;
}
