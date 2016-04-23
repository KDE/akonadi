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

#include "akonadiserver_debug.h"

using namespace Akonadi;
using namespace Akonadi::Server;

ItemRetriever::ItemRetriever(Connection *connection)
    : mScope()
    , mConnection(connection)
    , mFullPayload(false)
    , mRecursive(false)
{
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
    PimItemRidColumn,

    MimeTypeIdColumn,

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
    qb.addColumn(PimItem::remoteIdFullColumnName());
    qb.addColumn(PimItem::mimeTypeIdFullColumnName());
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

bool ItemRetriever::exec()
{
    if (mParts.isEmpty() && !mFullPayload) {
        return true;
    }

    verifyCache();

    QSqlQuery query = buildQuery();
    ItemRetrievalRequest *lastRequest = 0;
    QList<ItemRetrievalRequest *> requests;

    QByteArrayList parts;
    Q_FOREACH (const QByteArray &part, mParts) {
        if (part.startsWith(AKONADI_PARAM_PLD)) {
            parts << part.mid(4);
        }
    }

    QHash<qint64, QString> mimeTypeIdNameCache;
    QHash<qint64, QString> resourceIdNameCache;
    while (query.isValid()) {
        const qint64 pimItemId = query.value(PimItemIdColumn).toLongLong();
        if (!lastRequest || lastRequest->id != pimItemId) {
            lastRequest = new ItemRetrievalRequest();
            lastRequest->id = pimItemId;
            lastRequest->remoteId = Utils::variantToString(query.value(PimItemRidColumn));
            const qint64 mtId = query.value(MimeTypeIdColumn).toLongLong();
            auto mtIter = mimeTypeIdNameCache.find(mtId);
            if (mtIter == mimeTypeIdNameCache.end()) {
                mtIter = mimeTypeIdNameCache.insert(mtId, MimeType::retrieveById(mtId).name());
            }
            lastRequest->mimeType = *mtIter;
            const qint64 resourceId = query.value(ResourceIdColumn).toLongLong();
            auto resIter = resourceIdNameCache.find(resourceId);
            if (resIter == resourceIdNameCache.end()) {
                resIter = resourceIdNameCache.insert(resourceId, Resource::retrieveById(resourceId).name());
            }
            lastRequest->resourceId = *resIter;
            lastRequest->parts = parts;
            requests << lastRequest;
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
            Q_ASSERT(lastRequest->parts.count(partName) <= 1);
            // data available, don't request update
            lastRequest->parts.removeOne(partName);
        }
        query.next();
    }

    //qCDebug(AKONADISERVER_LOG) << "Closing queries and sending out requests.";

    query.finish();

    Q_FOREACH (ItemRetrievalRequest *request, requests) {
        if (request->parts.isEmpty()) {
            delete request;
            continue;
        }
        // TODO: how should we handle retrieval errors here? so far they have been ignored,
        // which makes sense in some cases, do we need a command parameter for this?
        try {
            ItemRetrievalManager::instance()->requestItemDelivery(request);
        } catch (const ItemRetrieverException &e) {
            qCCritical(AKONADISERVER_LOG) << e.type() << ": " << e.what();
            mLastError = e.what();
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
    if (!connection()->verifyCacheOnRetrieval()) {
        return;
    }

    SelectQueryBuilder<Part> qb;
    qb.addJoin(QueryBuilder::InnerJoin, PimItem::tableName(), Part::pimItemIdFullColumnName(), PimItem::idFullColumnName());
    qb.addValueCondition(Part::externalFullColumnName(), Query::Equals, true);
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
    Q_FOREACH (Part part, externalParts) {
        PartHelper::verify(part);
    }
}

QByteArray ItemRetriever::lastError() const
{
    return mLastError;
}
