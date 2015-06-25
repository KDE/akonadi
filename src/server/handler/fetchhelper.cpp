/***************************************************************************
 *   Copyright (C) 2006-2009 by Tobias Koenig <tokoe@kde.org>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "fetchhelper.h"

#include "akonadi.h"
#include "connection.h"
#include "handler.h"
#include "handlerhelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretrievalmanager.h"
#include "storage/itemretrievalrequest.h"
#include "storage/parthelper.h"
#include "storage/parttypehelper.h"
#include "storage/transaction.h"
#include "utils.h"
#include "intervalcheck.h"
#include "agentmanagerinterface.h"
#include "dbusconnectionpool.h"
#include "tagfetchhelper.h"
#include "relationfetch.h"

#include <private/scope_p.h>

#include <shared/akdebug.h>
#include <shared/akdbus.h>

#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Akonadi;
using namespace Akonadi::Server;

FetchHelper::FetchHelper(Connection *connection, const Scope &scope,
                         const Protocol::FetchScope &fetchScope)
    : mConnection(connection)
    , mScope(scope)
    , mFetchScope(fetchScope)
{
    std::fill(mItemQueryColumnMap, mItemQueryColumnMap + ItemQueryColumnCount, -1);
}

enum PartQueryColumns {
    PartQueryPimIdColumn,
    PartQueryTypeNamespaceColumn,
    PartQueryTypeNameColumn,
    PartQueryDataColumn,
    PartQueryExternalColumn,
    PartQueryVersionColumn,
    PartQueryDataSizeColumn
};

QSqlQuery FetchHelper::buildPartQuery(const QVector<QByteArray> &partList, bool allPayload, bool allAttrs)
{
    ///TODO: merge with ItemQuery
    QueryBuilder partQuery(PimItem::tableName());

    if (!partList.isEmpty() || allPayload || allAttrs) {
        partQuery.addJoin(QueryBuilder::InnerJoin, Part::tableName(), PimItem::idFullColumnName(), Part::pimItemIdFullColumnName());
        partQuery.addJoin(QueryBuilder::InnerJoin, PartType::tableName(), Part::partTypeIdFullColumnName(), PartType::idFullColumnName());
        partQuery.addColumn(PimItem::idFullColumnName());
        partQuery.addColumn(PartType::nsFullColumnName());
        partQuery.addColumn(PartType::nameFullColumnName());
        partQuery.addColumn(Part::dataFullColumnName());
        partQuery.addColumn(Part::externalFullColumnName());
        partQuery.addColumn(Part::versionFullColumnName());
        partQuery.addColumn(Part::datasizeFullColumnName());

        partQuery.addSortColumn(PimItem::idFullColumnName(), Query::Descending);

        Query::Condition cond(Query::Or);
        if (!partList.isEmpty()) {
            QStringList partNameList;
            Q_FOREACH (const QByteArray &b, partList) {
                if (b.startsWith("PLD") || b.startsWith("ATR")) {
                    partNameList.push_back(QString::fromLatin1(b));
                }
            }
            if (!partNameList.isEmpty()) {
                cond.addCondition(PartTypeHelper::conditionFromFqNames(partNameList));
            }
        }

        if (allPayload) {
            cond.addValueCondition(PartType::nsFullColumnName(), Query::Equals, QLatin1String("PLD"));
        }
        if (allAttrs) {
            cond.addValueCondition(PartType::nsFullColumnName(), Query::Equals, QLatin1String("ATR"));
        }

        if (!cond.isEmpty()) {
            partQuery.addCondition(cond);
        }

        ItemQueryHelper::scopeToQuery(mScope, mConnection->context(), partQuery);

        if (!partQuery.exec()) {
            throw HandlerException("Unable to list item parts");
        }
        partQuery.query().next();
    }

    return partQuery.query();
}

QSqlQuery FetchHelper::buildItemQuery()
{
    QueryBuilder itemQuery(PimItem::tableName());

    itemQuery.addJoin(QueryBuilder::InnerJoin, MimeType::tableName(),
                      PimItem::mimeTypeIdFullColumnName(), MimeType::idFullColumnName());

    int column = 0;
#define ADD_COLUMN(colName, colId) { itemQuery.addColumn( colName ); mItemQueryColumnMap[colId] = column++; }
    ADD_COLUMN(PimItem::idFullColumnName(), ItemQueryPimItemIdColumn);
    if (mFetchScope.fetchRemoteId()) {
        ADD_COLUMN(PimItem::remoteIdFullColumnName(), ItemQueryPimItemRidColumn)
    }
    ADD_COLUMN(MimeType::nameFullColumnName(), ItemQueryMimeTypeColumn)
    ADD_COLUMN(PimItem::revFullColumnName(), ItemQueryRevColumn)
    if (mFetchScope.fetchRemoteRevision()) {
        ADD_COLUMN(PimItem::remoteRevisionFullColumnName(), ItemQueryRemoteRevisionColumn)
    }
    if (mFetchScope.fetchSize()) {
        ADD_COLUMN(PimItem::sizeFullColumnName(), ItemQuerySizeColumn)
    }
    if (mFetchScope.fetchMTime()) {
        ADD_COLUMN(PimItem::datetimeFullColumnName(), ItemQueryDatetimeColumn)
    }
    ADD_COLUMN(PimItem::collectionIdFullColumnName(), ItemQueryCollectionIdColumn)
    if (mFetchScope.fetchGID()) {
        ADD_COLUMN(PimItem::gidFullColumnName(), ItemQueryPimItemGidColumn)
    }
#undef ADD_COLUMN

    itemQuery.addSortColumn(PimItem::idFullColumnName(), Query::Descending);

    ItemQueryHelper::scopeToQuery(mScope, mConnection->context(), itemQuery);

    if (mFetchScope.changedSince().isValid()) {
        itemQuery.addValueCondition(PimItem::datetimeFullColumnName(), Query::GreaterOrEqual, mFetchScope.changedSince().toUTC());
    }

    if (!itemQuery.exec()) {
        throw HandlerException("Unable to list items");
    }

    itemQuery.query().next();

    return itemQuery.query();
}

enum FlagQueryColumns {
    FlagQueryIdColumn,
    FlagQueryNameColumn
};

QSqlQuery FetchHelper::buildFlagQuery()
{
    QueryBuilder flagQuery(PimItem::tableName());
    flagQuery.addJoin(QueryBuilder::InnerJoin, PimItemFlagRelation::tableName(),
                      PimItem::idFullColumnName(), PimItemFlagRelation::leftFullColumnName());
    flagQuery.addJoin(QueryBuilder::InnerJoin, Flag::tableName(),
                      Flag::idFullColumnName(), PimItemFlagRelation::rightFullColumnName());
    flagQuery.addColumn(PimItem::idFullColumnName());
    flagQuery.addColumn(Flag::nameFullColumnName());

    ItemQueryHelper::scopeToQuery(mScope, mConnection->context(), flagQuery);
    flagQuery.addSortColumn(PimItem::idFullColumnName(), Query::Descending);

    if (!flagQuery.exec()) {
        throw HandlerException("Unable to retrieve item flags");
    }

    flagQuery.query().next();

    return flagQuery.query();
}

enum TagQueryColumns {
    TagQueryItemIdColumn,
    TagQueryTagIdColumn,
};

QSqlQuery FetchHelper::buildTagQuery()
{
    QueryBuilder tagQuery(PimItem::tableName());
    tagQuery.addJoin(QueryBuilder::InnerJoin, PimItemTagRelation::tableName(),
                     PimItem::idFullColumnName(), PimItemTagRelation::leftFullColumnName());
    tagQuery.addJoin(QueryBuilder::InnerJoin, Tag::tableName(),
                     Tag::idFullColumnName(), PimItemTagRelation::rightFullColumnName());
    tagQuery.addColumn(PimItem::idFullColumnName());
    tagQuery.addColumn(Tag::idFullColumnName());

    ItemQueryHelper::scopeToQuery(mScope, mConnection->context(), tagQuery);
    tagQuery.addSortColumn(PimItem::idFullColumnName(), Query::Descending);

    if (!tagQuery.exec()) {
        throw HandlerException("Unable to retrieve item tags");
    }

    tagQuery.query().next();

    return tagQuery.query();
}

enum VRefQueryColumns {
    VRefQueryCollectionIdColumn,
    VRefQueryItemIdColumn
};

QSqlQuery FetchHelper::buildVRefQuery()
{
    QueryBuilder vRefQuery(PimItem::tableName());
    vRefQuery.addJoin(QueryBuilder::LeftJoin, CollectionPimItemRelation::tableName(),
                      CollectionPimItemRelation::rightFullColumnName(),
                      PimItem::idFullColumnName());
    vRefQuery.addColumn(CollectionPimItemRelation::leftFullColumnName());
    vRefQuery.addColumn(CollectionPimItemRelation::rightFullColumnName());
    ItemQueryHelper::scopeToQuery(mScope, mConnection->context(), vRefQuery);
    vRefQuery.addSortColumn(PimItem::idFullColumnName(), Query::Descending);

    if (!vRefQuery.exec()) {
        throw HandlerException("Unable to retrieve virtual references");
    }

    vRefQuery.query().next();

    return vRefQuery.query();
}

bool FetchHelper::isScopeLocal(const Scope &scope)
{
    // The only agent allowed to override local scope is the Baloo Indexer
    if (!mConnection->sessionId().startsWith("akonadi_baloo_indexer")) {
        return false;
    }

    // Get list of all resources that own all items in the scope
    QueryBuilder qb(PimItem::tableName(), QueryBuilder::Select);
    qb.setDistinct(true);
    qb.addColumn(Resource::nameFullColumnName());
    qb.addJoin(QueryBuilder::LeftJoin, Collection::tableName(),
               PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
    qb.addJoin(QueryBuilder::LeftJoin, Resource::tableName(),
               Collection::resourceIdFullColumnName(), Resource::idFullColumnName());
    ItemQueryHelper::scopeToQuery(scope, mConnection->context(), qb);
    if (mConnection->context()->resource().isValid()) {
        qb.addValueCondition(Resource::nameFullColumnName(), Query::NotEquals,
                             mConnection->context()->resource().name());
    }

    if (!qb.exec()) {
        throw HandlerException("Failed to query database");
        return false;
    }

    // If there is more than one resource, i.e. this is a fetch from multiple
    // collections, then don't bother and just return FALSE. This case is aimed
    // specifically on Baloo, which fetches items from each collection independently,
    // so it will pass this check.
    QSqlQuery query = qb.query();
    if (query.size() != 1) {
        return false;
    }

    query.next();
    const QString resourceName = query.value(0).toString();

    org::freedesktop::Akonadi::AgentManager manager(AkDBus::serviceName(AkDBus::Control),
                                                    QLatin1String("/AgentManager"),
                                                    DBusConnectionPool::threadConnection());
    const QString typeIdentifier = manager.agentInstanceType(resourceName);
    const QVariantMap properties = manager.agentCustomProperties(typeIdentifier);
    return properties.value(QLatin1String("HasLocalStorage"), false).toBool();
}

bool FetchHelper::fetchItems()
{
    // retrieve missing parts
    // HACK: isScopeLocal() is a workaround for resources that have cache expiration
    // because when the cache expires, Baloo is not able to content of the items. So
    // we allow fetch of items that belong to local resources (like maildir) to ignore
    // cacheOnly and retrieve missing parts from the resource. However ItemRetriever
    // is painfully slow with many items and is generally designed to fetch a few
    // messages, not all of them. In the long term, we need a better way to do this.
    if (!mFetchScope.cacheOnly() || isScopeLocal(mScope)) {
        // trigger a collection sync if configured to do so
        triggerOnDemandFetch();

        // Prepare for a call to ItemRetriever::exec();
        // From a resource perspective the only parts that can be fetched are payloads.
        ItemRetriever retriever(mConnection);
        retriever.setScope(mScope);
        retriever.setRetrieveParts(mFetchScope.requestedPayloads());
        retriever.setRetrieveFullPayload(mFetchScope.fullPayload());
        retriever.setChangedSince(mFetchScope.changedSince());
        if (!retriever.exec() && !mFetchScope.ignoreErrors()) {   // There we go, retrieve the missing parts from the resource.
            if (mConnection->context()->resource().isValid()) {
                throw HandlerException(QString::fromLatin1("Unable to fetch item from backend (collection %1, resource %2) : %3")
                                       .arg(mConnection->context()->collectionId())
                                       .arg(mConnection->context()->resource().id())
                                       .arg(QString::fromLatin1(retriever.lastError())));
            } else {
                throw HandlerException(QString::fromLatin1("Unable to fetch item from backend (collection %1) : %2")
                                       .arg(mConnection->context()->collectionId())
                                       .arg(QString::fromLatin1(retriever.lastError())));
            }
        }
    }

    QSqlQuery itemQuery = buildItemQuery();

    // error if query did not find any item and scope is not listing items but
    // a request for a specific item
    if (!itemQuery.isValid()) {
        if (mFetchScope.ignoreErrors()) {
            return true;
        }
        switch (mScope.scope()) {
        case Scope::Uid: // fall through
        case Scope::Rid: // fall through
        case Scope::HierarchicalRid: // fall through
        case Scope::Gid:
            throw HandlerException("Item query returned empty result set");
            break;
        default:
            break;
        }
    }
    // build part query if needed
    QSqlQuery partQuery;
    if (!mFetchScope.requestedParts().isEmpty() || mFetchScope.fullPayload() || mFetchScope.allAttributes()) {
        partQuery = buildPartQuery(mFetchScope.requestedParts(), mFetchScope.fullPayload(), mFetchScope.allAttributes());
    }

    // build flag query if needed
    QSqlQuery flagQuery;
    if (mFetchScope.fetchFlags()) {
        flagQuery = buildFlagQuery();
    }

    // build tag query if needed
    QSqlQuery tagQuery;
    if (mFetchScope.fetchTags()) {
        tagQuery = buildTagQuery();
    }

    QSqlQuery vRefQuery;
    if (mFetchScope.fetchVirtualReferences()) {
        vRefQuery = buildVRefQuery();
    }

    while (itemQuery.isValid()) {
        const qint64 pimItemId = extractQueryResult(itemQuery, ItemQueryPimItemIdColumn).toLongLong();
        const int pimItemRev = extractQueryResult(itemQuery, ItemQueryRevColumn).toInt();

        Protocol::FetchItemsResponse response(pimItemId);
        response.setRevision(pimItemRev);
        response.setMimeType(extractQueryResult(itemQuery, ItemQueryMimeTypeColumn).toString());
        if (mFetchScope.fetchRemoteId()) {
            response.setRemoteId(extractQueryResult(itemQuery, ItemQueryPimItemRidColumn).toString());
        }
        response.setParentId(extractQueryResult(itemQuery, ItemQueryCollectionIdColumn).toLongLong());

        if (mFetchScope.fetchSize()) {
            response.setSize(extractQueryResult(itemQuery, ItemQuerySizeColumn).toLongLong());
        }
        if (mFetchScope.fetchMTime()) {
            response.setMTime(Utils::variantToDateTime(extractQueryResult(itemQuery, ItemQueryDatetimeColumn)));
        }
        if (mFetchScope.fetchRemoteRevision()) {
            response.setRemoteRevision(extractQueryResult(itemQuery, ItemQueryRemoteRevisionColumn).toString());
        }
        if (mFetchScope.fetchGID()) {
            response.setGid(extractQueryResult(itemQuery, ItemQueryPimItemGidColumn).toString());
        }

        if (mFetchScope.fetchFlags()) {
            QVector<QByteArray> flags;
            while (flagQuery.isValid()) {
                const qint64 id = flagQuery.value(FlagQueryIdColumn).toLongLong();
                if (id > pimItemId) {
                    flagQuery.next();
                    continue;
                } else if (id < pimItemId) {
                    break;
                }
                flags << flagQuery.value(FlagQueryNameColumn).toByteArray();
                flagQuery.next();
            }
            response.setFlags(flags);
        }

        if (mFetchScope.fetchTags()) {
            QVector<qint64> tagIds;
            QVector<Protocol::FetchTagsResponse> tags;
            //We don't take the fetch scope into account yet. It's either id only or the full tag.
            const bool fullTagsRequested = !mFetchScope.tagFetchScope().isEmpty();
            while (tagQuery.isValid()) {
                const qint64 id = tagQuery.value(TagQueryItemIdColumn).toLongLong();
                if (id > pimItemId) {
                    tagQuery.next();
                    continue;
                } else if (id < pimItemId) {
                    break;
                }
                tagIds << tagQuery.value(TagQueryTagIdColumn).toLongLong();
                tagQuery.next();
            }
            if (!fullTagsRequested) {
                Q_FOREACH (qint64 tagId, tagIds) {
                    tags << Protocol::FetchTagsResponse(tagId);
                }
            } else {
                tags.reserve(tagIds.count());
                Q_FOREACH (qint64 tagId, tagIds) {
                    tags << HandlerHelper::fetchTagsResponse(Tag::retrieveById(tagId));
                }
            }
            response.setTags(tags);
        }

        if (mFetchScope.fetchVirtualReferences()) {
            QVector<qint64> vRefs;
            while (vRefQuery.isValid()) {
                const qint64 id = vRefQuery.value(VRefQueryItemIdColumn).toLongLong();
                if (id > pimItemId) {
                    vRefQuery.next();
                    continue;
                } else if (id < pimItemId) {
                    break;
                }
                vRefs << vRefQuery.value(VRefQueryCollectionIdColumn).toLongLong();
                vRefQuery.next();
            }
            response.setVirtualReferences(vRefs);
        }

        if (mFetchScope.fetchRelations()) {
            SelectQueryBuilder<Relation> qb;
            Query::Condition condition;
            condition.setSubQueryMode(Query::Or);
            condition.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, pimItemId);
            condition.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, pimItemId);
            qb.addCondition(condition);
            qb.addGroupColumns(QStringList() << Relation::leftIdColumn() << Relation::rightIdColumn() << Relation::typeIdColumn() << Relation::remoteIdColumn());
            if (!qb.exec()) {
                throw HandlerException("Unable to list item relations");
            }
            QVector<Protocol::FetchRelationsResponse> relations;
            Q_FOREACH (const Relation &rel, qb.result()) {
                relations << HandlerHelper::fetchRelationsResponse(rel);
            }
            response.setRelations(relations);
        }

        if (mFetchScope.ancestorDepth() != Protocol::Ancestor::NoAncestor) {
            response.setAncestors(ancestorsForItem(response.parentId()));
        }

        bool skipItem = false;

        QVector<QByteArray> cachedParts;
        QVector<Protocol::StreamPayloadResponse> parts;
        while (partQuery.isValid()) {
            const qint64 id = partQuery.value(PartQueryPimIdColumn).toLongLong();
            if (id > pimItemId) {
                partQuery.next();
                continue;
            } else if (id < pimItemId) {
                break;
            }

            const QByteArray partName
                = Utils::variantToByteArray(partQuery.value(PartQueryTypeNamespaceColumn))
                    + ":"+ Utils::variantToByteArray(partQuery.value(PartQueryTypeNameColumn));

            Protocol::PartMetaData metaPart;
            Protocol::StreamPayloadResponse partData;
            partData.setPayloadName(partName);
            metaPart.setName(partName);
            metaPart.setVersion(partQuery.value(PartQueryVersionColumn).toInt());
            metaPart.setSize(partQuery.value(PartQueryDataSizeColumn).toLongLong());

            const QByteArray data = Utils::variantToByteArray(partQuery.value(PartQueryDataColumn));
            if (mFetchScope.checkCachedPayloadPartsOnly()) {
                if (!data.isEmpty()) {
                    cachedParts << partName;
                }
                partQuery.next();
            } else {
                if (mFetchScope.ignoreErrors() && data.isEmpty()) {
                    //We wanted the payload, couldn't get it, and are ignoring errors. Skip the item.
                    //This is not an error though, it's fine to have empty payload parts (to denote existing but not cached parts)
                    //akDebug() << "item" << id << "has an empty payload part in parttable for part" << partName;
                    skipItem = true;
                    break;
                }
                metaPart.setIsExternal(partQuery.value(PartQueryExternalColumn).toBool());
                if (data.isEmpty()) {
                    partData.setData(QByteArray(""));
                } else {
                    partData.setData(data);
                }
                partData.setMetaData(metaPart);

                if (mFetchScope.requestedParts().contains(partName) || mFetchScope.fullPayload() || mFetchScope.allAttributes()) {
                    parts.append(partData);
                }

                partQuery.next();
            }
        }
        response.setParts(parts);

        if (skipItem) {
            itemQuery.next();
            continue;
        }

        if (mFetchScope.checkCachedPayloadPartsOnly()) {
            response.setCachedParts(cachedParts);
        }

        mConnection->sendResponse(response);

        itemQuery.next();
    }

    // update atime (only if the payload was actually requested, otherwise a simple resource sync prevents cache clearing)
    if (needsAccessTimeUpdate(mFetchScope.requestedParts()) || mFetchScope.fullPayload()) {
        updateItemAccessTime();
    }

    return true;
}

bool FetchHelper::needsAccessTimeUpdate(const QVector<QByteArray> &parts)
{
    // TODO technically we should compare the part list with the cache policy of
    // the parent collection of the retrieved items, but that's kinda expensive
    // Only updating the atime if the full payload was requested is a good
    // approximation though.
    return parts.contains(AKONADI_PARAM_PLD_RFC822);
}

void FetchHelper::updateItemAccessTime()
{
    Transaction transaction(mConnection->storageBackend());
    QueryBuilder qb(PimItem::tableName(), QueryBuilder::Update);
    qb.setColumnValue(PimItem::atimeColumn(), QDateTime::currentDateTimeUtc());
    ItemQueryHelper::scopeToQuery(mScope, mConnection->context(), qb);

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Unable to update item access time";
    } else {
        transaction.commit();
    }
}

void FetchHelper::triggerOnDemandFetch()
{
    if (mConnection->context()->collectionId() <= 0 || mFetchScope.cacheOnly()) {
        return;
    }

    Collection collection = mConnection->context()->collection();

    // HACK: don't trigger on-demand syncing if the resource is the one triggering it
    if (mConnection->sessionId() == collection.resource().name().toLatin1()) {
        return;
    }

    DataStore *store = mConnection->storageBackend();
    store->activeCachePolicy(collection);
    if (!collection.cachePolicySyncOnDemand()) {
        return;
    }

    if (AkonadiServer::instance()->intervalChecker()) {
        AkonadiServer::instance()->intervalChecker()->requestCollectionSync(collection);
    }
}

QVector<Protocol::Ancestor> FetchHelper::ancestorsForItem(Collection::Id parentColId)
{
    if (mFetchScope.ancestorDepth() == Protocol::Ancestor::NoAncestor || parentColId == 0) {
        return QVector<Protocol::Ancestor>();
    }
    if (mAncestorCache.contains(parentColId)) {
        return mAncestorCache.value(parentColId);
    }

    QVector<Protocol::Ancestor> ancestors;
    Collection col = Collection::retrieveById(parentColId);
    const int depthNum = mFetchScope.ancestorDepth() == Protocol::Ancestor::ParentAncestor ? 1 : INT_MAX;
    for (int i = 0; i < depthNum; ++i) {
        if (!col.isValid()) {
            ancestors << Protocol::Ancestor(0);
            break;
        }
        ancestors << Protocol::Ancestor(col.id(), col.remoteId());
        col = col.parent();
    }
    mAncestorCache.insert(parentColId, ancestors);
    return ancestors;
}

QVariant FetchHelper::extractQueryResult(const QSqlQuery &query, FetchHelper::ItemQueryColumns column) const
{
    Q_ASSERT(mItemQueryColumnMap[column] >= 0);
    return query.value(mItemQueryColumnMap[column]);
}
