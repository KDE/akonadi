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


#include <private/protocol_p.h>

#include <QEventLoop>

#include "akonadiserver_debug.h"

using namespace Akonadi;
using namespace Akonadi::Server;

ItemRetriever::ItemRetriever(Connection *connection)
    : mScope()
    , mConnection(connection)
    , mFullPayload(false)
    , mRecursive(false)
    , mCanceled(false)
{
    connect(mConnection, &Connection::disconnected,
    this, [this]() {
        mCanceled = true;
    });
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
    Q_ASSERT(mConnection);
    if (!isUid && mConnection->context()->collectionId() >= 0) {
        setItemSet(set, mConnection->context()->collection());
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
        ItemQueryHelper::scopeToQuery(mScope, mConnection->context(), qb);
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
static bool hasAllParts(ItemRetrievalRequest *req, const QSet<QByteArray> &availableParts)
{
    for (const auto &part : qAsConst(req->parts)) {
        if (!availableParts.contains(part)) {
            return false;
        }
    }
    return true;
}
}

bool ItemRetriever::exec()
{
    if (mParts.isEmpty() && !mFullPayload) {
        return true;
    }

    verifyCache();

    QSqlQuery query = buildQuery();
    QByteArrayList parts;
    for (const QByteArray &part : qAsConst(mParts)) {
        if (part.startsWith(AKONADI_PARAM_PLD)) {
            parts << part.mid(4);
        }
    }

    QHash<qint64, QString> resourceIdNameCache;
    QVector<ItemRetrievalRequest *> requests;
    QHash<qint64 /* collection */, ItemRetrievalRequest *> colRequests;
    QHash<qint64 /* item */, ItemRetrievalRequest *> itemRequests;
    QVector<qint64> readyItems;
    qint64 prevPimItemId = -1;
    QSet<QByteArray> availableParts;
    ItemRetrievalRequest *lastRequest = nullptr;
    while (query.isValid()) {
        const qint64 pimItemId = query.value(PimItemIdColumn).toLongLong();
        const qint64 collectionId = query.value(CollectionIdColumn).toLongLong();
        const qint64 resourceId = query.value(ResourceIdColumn).toLongLong();
        const auto itemIter = itemRequests.constFind(pimItemId);

        if (Q_UNLIKELY(mCanceled)) {
            return false;
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
            if (lastRequest) {
                if (hasAllParts(lastRequest, availableParts)) {
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
            lastRequest = *itemIter;
        } else {
            lastRequest = colRequests.value(collectionId);
            if (!lastRequest || lastRequest->ids.size() > 100) {
                lastRequest = new ItemRetrievalRequest();
                lastRequest->ids.push_back(pimItemId);
                auto resIter = resourceIdNameCache.find(resourceId);
                if (resIter == resourceIdNameCache.end()) {
                    resIter = resourceIdNameCache.insert(resourceId, Resource::retrieveById(resourceId).name());
                }
                lastRequest->resourceId = *resIter;
                lastRequest->parts = parts;
                colRequests.insert(collectionId, lastRequest);
                itemRequests.insert(pimItemId, lastRequest);
                requests << lastRequest;
            } else {
                lastRequest->ids.push_back(pimItemId);
                itemRequests.insert(pimItemId, lastRequest);
            }
        }

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
                lastRequest->parts << partName;
            }
        } else {
            // add the part to list of available parts, we will compare it with
            // the list of request parts once we handle all parts of this item
            availableParts.insert(partName);
        }
        query.next();
    }

    // Post-check in case we only queried one item thus did not reach the check
    // at the beginning of the while() loop above
    if (lastRequest && hasAllParts(lastRequest, availableParts)) {
        lastRequest->ids.removeOne(prevPimItemId);
        readyItems.push_back(prevPimItemId);
        // No need to update the hashtable at this point
    }

    //qCDebug(AKONADISERVER_LOG) << "Closing queries and sending out requests.";
    query.finish();

    if (!readyItems.isEmpty()) {
        Q_EMIT itemsRetrieved(readyItems.toList());
    }

    QEventLoop eventLoop;
    connect(ItemRetrievalManager::instance(), &ItemRetrievalManager::requestFinished,
            this, [&](ItemRetrievalRequest *finishedRequest) {
        if (requests.removeOne(finishedRequest)) {
            if (mCanceled) {
                eventLoop.exit(1);
            } else if (!finishedRequest->errorMsg.isEmpty()) {
                mLastError = finishedRequest->errorMsg.toUtf8();
                eventLoop.exit(1);
            } else {
                Q_EMIT itemsRetrieved(finishedRequest->ids);
                if (requests.isEmpty()) {
                    eventLoop.quit();
                }
            }
        }
    }, Qt::UniqueConnection);
    connect(mConnection, &Connection::connectionClosing,
            &eventLoop, [&eventLoop]() { eventLoop.exit(1); });

    auto it = requests.begin();
    while (it != requests.end()) {
        auto request = (*it);
        if ((!mFullPayload && request->parts.isEmpty()) || request->ids.isEmpty()) {
            it = requests.erase(it);
            delete request;
            continue;
        }
        // TODO: how should we handle retrieval errors here? so far they have been ignored,
        // which makes sense in some cases, do we need a command parameter for this?
        try {
            // Request is deleted inside ItemRetrievalManager, so we need to take
            // a copy here
            const auto ids = request->ids;
            ItemRetrievalManager::instance()->requestItemDelivery(request);
        } catch (const ItemRetrieverException &e) {
            qCCritical(AKONADISERVER_LOG) << e.type() << ": " << e.what();
            mLastError = e.what();
            return false;
        }
        ++it;
    }
    if (!requests.isEmpty()) {
        if (eventLoop.exec(QEventLoop::ExcludeSocketNotifiers)) {
            return false;
        }
    }

    // retrieve items in child collections if requested
    bool result = true;
    if (mRecursive && mCollection.isValid()) {
        Q_FOREACH (const Collection &col, mCollection.children()) {
            ItemRetriever retriever(mConnection);
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
        ItemQueryHelper::scopeToQuery(mScope, mConnection->context(), qb);
    } else {
        ItemQueryHelper::itemSetToQuery(mItemSet, qb, mCollection);
    }

    if (!qb.exec()) {
        mLastError = "Unable to query parts.";
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
