
/***************************************************************************
 *   SPDX-FileCopyrightText: 2006-2009 Tobias Koenig <tokoe@kde.org>       *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "itemfetchhelper.h"

#include "akonadi.h"
#include "connection.h"
#include "handler.h"
#include "handlerhelper.h"
#include "shared/akranges.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretrievalmanager.h"
#include "storage/parttypehelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

#include "agentmanagerinterface.h"
#include "akonadiserver_debug.h"
#include "intervalcheck.h"
#include "utils.h"

#include "private/dbus_p.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

#include <QElapsedTimer>

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

#define ENABLE_FETCH_PROFILING 0
#if ENABLE_FETCH_PROFILING
#define BEGIN_TIMER(name)                                                                                                                                      \
    QElapsedTimer name##Timer;                                                                                                                                 \
    name##Timer.start();

#define END_TIMER(name) const double name##Elapsed = name##Timer.nsecsElapsed() / 1000000.0;
#define PROF_INC(name) ++name;
#else
#define BEGIN_TIMER(name)
#define END_TIMER(name)
#define PROF_INC(name)
#endif

ItemFetchHelper::ItemFetchHelper(Connection *connection,
                                 const Scope &scope,
                                 const Protocol::ItemFetchScope &itemFetchScope,
                                 const Protocol::TagFetchScope &tagFetchScope,
                                 AkonadiServer &akonadi,
                                 const Protocol::FetchLimit &itemsLimit)
    : ItemFetchHelper(connection, connection->context(), scope, itemFetchScope, tagFetchScope, akonadi, itemsLimit)
{
}

ItemFetchHelper::ItemFetchHelper(Connection *connection,
                                 const CommandContext &context,
                                 const Scope &scope,
                                 const Protocol::ItemFetchScope &itemFetchScope,
                                 const Protocol::TagFetchScope &tagFetchScope,
                                 AkonadiServer &akonadi,
                                 const Protocol::FetchLimit &itemsLimit)
    : mConnection(connection)
    , mContext(context)
    , mScope(scope)
    , mItemFetchScope(itemFetchScope)
    , mTagFetchScope(tagFetchScope)
    , mAkonadi(akonadi)
    , mItemsLimit(itemsLimit)
    , mPimItemQueryAlias(QLatin1StringView("pimItem_alias"))
{
    std::fill(mItemQueryColumnMap, mItemQueryColumnMap + ItemQueryColumnCount, -1);
}

void ItemFetchHelper::disableATimeUpdates()
{
    mUpdateATimeEnabled = false;
}

enum PartQueryColumns {
    PartQueryPimIdColumn,
    PartQueryTypeIdColumn,
    PartQueryDataColumn,
    PartQueryStorageColumn,
    PartQueryVersionColumn,
    PartQueryDataSizeColumn
};

QueryBuilder ItemFetchHelper::buildPartQuery(QSqlQuery &itemQuery, const QList<QByteArray> &partList, bool allPayload, bool allAttrs)
{
    /// TODO: merge with ItemQuery
    QueryBuilder partQuery = mItemsLimit.limit() > 0 ? QueryBuilder(itemQuery, mPimItemQueryAlias) : QueryBuilder(PimItem::tableName());

    if (!partList.isEmpty() || allPayload || allAttrs) {
        partQuery.addJoin(QueryBuilder::InnerJoin, Part::tableName(), partQuery.getTableWithColumn(PimItem::idColumn()), Part::pimItemIdFullColumnName());
        partQuery.addColumn(partQuery.getTableWithColumn(PimItem::idColumn()));
        partQuery.addColumn(Part::partTypeIdFullColumnName());
        partQuery.addColumn(Part::dataFullColumnName());
        partQuery.addColumn(Part::storageFullColumnName());
        partQuery.addColumn(Part::versionFullColumnName());
        partQuery.addColumn(Part::datasizeFullColumnName());

        partQuery.addSortColumn(partQuery.getTableWithColumn(PimItem::idColumn()), Query::Descending);

        if (!partList.isEmpty() || allPayload || allAttrs) {
            Query::Condition cond(Query::Or);
            for (const QByteArray &b : std::as_const(partList)) {
                if (b.startsWith("PLD") || b.startsWith("ATR")) {
                    cond.addValueCondition(Part::partTypeIdFullColumnName(), Query::Equals, PartTypeHelper::fromFqName(b).id());
                }
            }
            if (allPayload || allAttrs) {
                partQuery.addJoin(QueryBuilder::InnerJoin, PartType::tableName(), Part::partTypeIdFullColumnName(), PartType::idFullColumnName());
                if (allPayload) {
                    cond.addValueCondition(PartType::nsFullColumnName(), Query::Equals, QStringLiteral("PLD"));
                }
                if (allAttrs) {
                    cond.addValueCondition(PartType::nsFullColumnName(), Query::Equals, QStringLiteral("ATR"));
                }
            }

            partQuery.addCondition(cond);
        }

        ItemQueryHelper::scopeToQuery(mScope, mContext, partQuery);

        if (!partQuery.exec()) {
            throw HandlerException("Unable to list item parts");
        }
        partQuery.query().next();
    }

    return partQuery;
}

QueryBuilder ItemFetchHelper::buildItemQuery()
{
    int column = 0;
    QueryBuilder itemQuery(PimItem::tableName());
    // clang-format off
#define ADD_COLUMN(colName, colId) {       \
    itemQuery.addColumn(colName);          \
    mItemQueryColumnMap[colId] = column++; \
}
    // clang-format on
    ADD_COLUMN(PimItem::idFullColumnName(), ItemQueryPimItemIdColumn);
    if (mItemFetchScope.fetchRemoteId()) {
        ADD_COLUMN(PimItem::remoteIdFullColumnName(), ItemQueryPimItemRidColumn)
    }
    ADD_COLUMN(PimItem::mimeTypeIdFullColumnName(), ItemQueryMimeTypeIdColumn)
    ADD_COLUMN(PimItem::revFullColumnName(), ItemQueryRevColumn)
    if (mItemFetchScope.fetchRemoteRevision()) {
        ADD_COLUMN(PimItem::remoteRevisionFullColumnName(), ItemQueryRemoteRevisionColumn)
    }
    if (mItemFetchScope.fetchSize()) {
        ADD_COLUMN(PimItem::sizeFullColumnName(), ItemQuerySizeColumn)
    }
    if (mItemFetchScope.fetchMTime()) {
        ADD_COLUMN(PimItem::datetimeFullColumnName(), ItemQueryDatetimeColumn)
    }
    ADD_COLUMN(PimItem::collectionIdFullColumnName(), ItemQueryCollectionIdColumn)
    if (mItemFetchScope.fetchGID()) {
        ADD_COLUMN(PimItem::gidFullColumnName(), ItemQueryPimItemGidColumn)
    }
    Q_UNUSED(column); // to suppress cppcheck unreadVariable false positive
#undef ADD_COLUMN

    itemQuery.addSortColumn(PimItem::idFullColumnName(), static_cast<Query::SortOrder>(mItemsLimit.sortOrder()));
    if (mItemsLimit.limit() > 0) {
        itemQuery.setLimit(mItemsLimit.limit(), mItemsLimit.limitOffset());
    }

    ItemQueryHelper::scopeToQuery(mScope, mContext, itemQuery);

    if (mItemFetchScope.changedSince().isValid()) {
        itemQuery.addValueCondition(PimItem::datetimeFullColumnName(), Query::GreaterOrEqual, mItemFetchScope.changedSince().toUTC());
    }

    if (!itemQuery.exec()) {
        throw HandlerException("Unable to list items");
    }

    itemQuery.query().next();

    return itemQuery;
}

enum FlagQueryColumns {
    FlagQueryPimItemIdColumn,
    FlagQueryFlagIdColumn,
};

QueryBuilder ItemFetchHelper::buildFlagQuery(QSqlQuery &itemQuery)
{
    QueryBuilder flagQuery = mItemsLimit.limit() > 0 ? QueryBuilder(itemQuery, mPimItemQueryAlias) : QueryBuilder(PimItem::tableName());

    flagQuery.addJoin(QueryBuilder::InnerJoin,
                      PimItemFlagRelation::tableName(),
                      flagQuery.getTableWithColumn(PimItem::idColumn()),
                      PimItemFlagRelation::leftFullColumnName());

    flagQuery.addColumn(flagQuery.getTableWithColumn(PimItem::idColumn()));
    flagQuery.addColumn(PimItemFlagRelation::rightFullColumnName());

    ItemQueryHelper::scopeToQuery(mScope, mContext, flagQuery);
    flagQuery.addSortColumn(flagQuery.getTableWithColumn(PimItem::idColumn()), Query::Descending);

    if (!flagQuery.exec()) {
        throw HandlerException("Unable to retrieve item flags");
    }

    flagQuery.query().next();

    return flagQuery;
}

enum TagQueryColumns {
    TagQueryItemIdColumn,
    TagQueryTagIdColumn,
};

QueryBuilder ItemFetchHelper::buildTagQuery(QSqlQuery &itemQuery)
{
    QueryBuilder tagQuery(PimItem::tableName());
    if (mItemsLimit.limit() > 0) {
        tagQuery = QueryBuilder(itemQuery, mPimItemQueryAlias);
    }

    tagQuery.addJoin(QueryBuilder::InnerJoin,
                     PimItemTagRelation::tableName(),
                     tagQuery.getTableWithColumn(PimItem::idColumn()),
                     PimItemTagRelation::leftFullColumnName());
    tagQuery.addJoin(QueryBuilder::InnerJoin, Tag::tableName(), Tag::idFullColumnName(), PimItemTagRelation::rightFullColumnName());
    tagQuery.addColumn(tagQuery.getTableWithColumn(PimItem::idColumn()));
    tagQuery.addColumn(Tag::idFullColumnName());

    ItemQueryHelper::scopeToQuery(mScope, mContext, tagQuery);
    tagQuery.addSortColumn(tagQuery.getTableWithColumn(PimItem::idColumn()), Query::Descending);

    if (!tagQuery.exec()) {
        throw HandlerException("Unable to retrieve item tags");
    }

    tagQuery.query().next();

    return tagQuery;
}

enum VRefQueryColumns {
    VRefQueryCollectionIdColumn,
    VRefQueryItemIdColumn,
};

QueryBuilder ItemFetchHelper::buildVRefQuery(QSqlQuery &itemQuery)
{
    QueryBuilder vRefQuery(PimItem::tableName());
    if (mItemsLimit.limit() > 0) {
        vRefQuery = QueryBuilder(itemQuery, mPimItemQueryAlias);
    }

    vRefQuery.addJoin(QueryBuilder::LeftJoin,
                      CollectionPimItemRelation::tableName(),
                      CollectionPimItemRelation::rightFullColumnName(),
                      vRefQuery.getTableWithColumn(PimItem::idColumn()));
    vRefQuery.addColumn(CollectionPimItemRelation::leftFullColumnName());
    vRefQuery.addColumn(CollectionPimItemRelation::rightFullColumnName());
    ItemQueryHelper::scopeToQuery(mScope, mContext, vRefQuery);
    vRefQuery.addSortColumn(vRefQuery.getTableWithColumn(PimItem::idColumn()), Query::Descending);

    if (!vRefQuery.exec()) {
        throw HandlerException("Unable to retrieve virtual references");
    }

    vRefQuery.query().next();

    return vRefQuery;
}

bool ItemFetchHelper::isScopeLocal(const Scope &scope)
{
    // The only agent allowed to override local scope is the Baloo Indexer
    if (!mConnection->sessionId().startsWith("akonadi_indexing_agent")) {
        return false;
    }

    // Get list of all resources that own all items in the scope
    QueryBuilder qb(PimItem::tableName(), QueryBuilder::Select);
    qb.setDistinct(true);
    qb.addColumn(Resource::nameFullColumnName());
    qb.addJoin(QueryBuilder::LeftJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
    qb.addJoin(QueryBuilder::LeftJoin, Resource::tableName(), Collection::resourceIdFullColumnName(), Resource::idFullColumnName());
    ItemQueryHelper::scopeToQuery(scope, mContext, qb);
    if (mContext.resource().isValid()) {
        qb.addValueCondition(Resource::nameFullColumnName(), Query::NotEquals, mContext.resource().name());
    }

    if (!qb.exec()) {
        throw HandlerException("Failed to query database");
        return false;
    }

    // If there is more than one resource, i.e. this is a fetch from multiple
    // collections, then don't bother and just return FALSE. This case is aimed
    // specifically on Baloo, which fetches items from each collection independently,
    // so it will pass this check.
    QSqlQuery &query = qb.query();
    if (query.size() != 1) {
        return false;
    }

    query.next();
    const QString resourceName = query.value(0).toString();
    query.finish();

    org::freedesktop::Akonadi::AgentManager manager(DBus::serviceName(DBus::Control), QStringLiteral("/AgentManager"), QDBusConnection::sessionBus());
    const QString typeIdentifier = manager.agentInstanceType(resourceName);
    const QVariantMap properties = manager.agentCustomProperties(typeIdentifier);
    return properties.value(QStringLiteral("HasLocalStorage"), false).toBool();
}

DataStore *ItemFetchHelper::storageBackend() const
{
    if (mConnection) {
        if (auto store = mConnection->storageBackend()) {
            return store;
        }
    }

    return DataStore::self();
}

bool ItemFetchHelper::fetchItems(std::function<void(Protocol::FetchItemsResponse &&)> &&itemCallback)
{
    BEGIN_TIMER(fetch)

    // retrieve missing parts
    // HACK: isScopeLocal() is a workaround for resources that have cache expiration
    // because when the cache expires, Baloo is not able to content of the items. So
    // we allow fetch of items that belong to local resources (like maildir) to ignore
    // cacheOnly and retrieve missing parts from the resource. However ItemRetriever
    // is painfully slow with many items and is generally designed to fetch a few
    // messages, not all of them. In the long term, we need a better way to do this.
    BEGIN_TIMER(itemRetriever)
    BEGIN_TIMER(scopeLocal)
#if ENABLE_FETCH_PROFILING
    double scopeLocalElapsed = 0;
#endif
    if (!mItemFetchScope.cacheOnly() || isScopeLocal(mScope)) {
#if ENABLE_FETCH_PROFILING
        scopeLocalElapsed = scopeLocalTimer.elapsed();
#endif

        // trigger a collection sync if configured to do so
        triggerOnDemandFetch();

        // Prepare for a call to ItemRetriever::exec();
        // From a resource perspective the only parts that can be fetched are payloads.
        ItemRetriever retriever(mAkonadi.itemRetrievalManager(), mConnection, mContext);
        retriever.setScope(mScope);
        retriever.setRetrieveParts(mItemFetchScope.requestedPayloads());
        retriever.setRetrieveFullPayload(mItemFetchScope.fullPayload());
        retriever.setChangedSince(mItemFetchScope.changedSince());
        if (!retriever.exec() && !mItemFetchScope.ignoreErrors()) { // There we go, retrieve the missing parts from the resource.
            if (mContext.resource().isValid()) {
                throw HandlerException(QStringLiteral("Unable to fetch item from backend (collection %1, resource %2) : %3")
                                           .arg(mContext.collectionId())
                                           .arg(mContext.resource().id())
                                           .arg(QString::fromLatin1(retriever.lastError())));
            } else {
                throw HandlerException(QStringLiteral("Unable to fetch item from backend (collection %1) : %2")
                                           .arg(mContext.collectionId())
                                           .arg(QString::fromLatin1(retriever.lastError())));
            }
        }
    }
    END_TIMER(itemRetriever)

    BEGIN_TIMER(items)
    std::optional<QueryBuilder> itemQb = buildItemQuery();
    auto &itemQuery = itemQb->query();
    END_TIMER(items)

    // error if query did not find any item and scope is not listing items but
    // a request for a specific item
    if (!itemQuery.isValid()) {
        if (mItemFetchScope.ignoreErrors()) {
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
    BEGIN_TIMER(parts)
    std::optional<QueryBuilder> partQb;
    if (!mItemFetchScope.requestedParts().isEmpty() || mItemFetchScope.fullPayload() || mItemFetchScope.allAttributes()) {
        partQb = buildPartQuery(itemQuery, mItemFetchScope.requestedParts(), mItemFetchScope.fullPayload(), mItemFetchScope.allAttributes());
    }
    END_TIMER(parts)

    // build flag query if needed
    BEGIN_TIMER(flags)
    std::optional<QueryBuilder> flagQb;
    if (mItemFetchScope.fetchFlags()) {
        flagQb = buildFlagQuery(itemQuery);
    }
    END_TIMER(flags)

    // build tag query if needed
    BEGIN_TIMER(tags)
    std::optional<QueryBuilder> tagQb;
    if (mItemFetchScope.fetchTags()) {
        tagQb = buildTagQuery(itemQuery);
    }
    END_TIMER(tags)

    BEGIN_TIMER(vRefs)
    std::optional<QueryBuilder> vRefQb;
    if (mItemFetchScope.fetchVirtualReferences()) {
        vRefQb = buildVRefQuery(itemQuery);
    }
    END_TIMER(vRefs)

#if ENABLE_FETCH_PROFILING
    int itemsCount = 0;
    int flagsCount = 0;
    int partsCount = 0;
    int tagsCount = 0;
    int vRefsCount = 0;
#endif

    BEGIN_TIMER(processing)
    QHash<qint64, QByteArray> flagIdNameCache;
    QHash<qint64, QString> mimeTypeIdNameCache;
    QHash<qint64, QByteArray> partTypeIdNameCache;
    while (itemQuery.isValid()) {
        PROF_INC(itemsCount)

        const qint64 pimItemId = extractQueryResult(itemQuery, ItemQueryPimItemIdColumn).toLongLong();
        const int pimItemRev = extractQueryResult(itemQuery, ItemQueryRevColumn).toInt();

        Protocol::FetchItemsResponse response;
        response.setId(pimItemId);
        response.setRevision(pimItemRev);
        const qint64 mimeTypeId = extractQueryResult(itemQuery, ItemQueryMimeTypeIdColumn).toLongLong();
        auto mtIter = mimeTypeIdNameCache.find(mimeTypeId);
        if (mtIter == mimeTypeIdNameCache.end()) {
            mtIter = mimeTypeIdNameCache.insert(mimeTypeId, MimeType::retrieveById(mimeTypeId).name());
        }
        response.setMimeType(mtIter.value());
        if (mItemFetchScope.fetchRemoteId()) {
            response.setRemoteId(extractQueryResult(itemQuery, ItemQueryPimItemRidColumn).toString());
        }
        response.setParentId(extractQueryResult(itemQuery, ItemQueryCollectionIdColumn).toLongLong());

        if (mItemFetchScope.fetchSize()) {
            response.setSize(extractQueryResult(itemQuery, ItemQuerySizeColumn).toLongLong());
        }
        if (mItemFetchScope.fetchMTime()) {
            response.setMTime(extractQueryResult(itemQuery, ItemQueryDatetimeColumn).toDateTime());
        }
        if (mItemFetchScope.fetchRemoteRevision()) {
            response.setRemoteRevision(extractQueryResult(itemQuery, ItemQueryRemoteRevisionColumn).toString());
        }
        if (mItemFetchScope.fetchGID()) {
            response.setGid(extractQueryResult(itemQuery, ItemQueryPimItemGidColumn).toString());
        }

        if (mItemFetchScope.fetchFlags() && flagQb) {
            QList<QByteArray> flags;
            auto &flagQuery = flagQb->query();
            while (flagQuery.isValid()) {
                const qint64 id = flagQuery.value(FlagQueryPimItemIdColumn).toLongLong();
                if (id > pimItemId) {
                    flagQuery.next();
                    continue;
                } else if (id < pimItemId) {
                    break;
                }
                const qint64 flagId = flagQuery.value(FlagQueryFlagIdColumn).toLongLong();
                auto flagNameIter = flagIdNameCache.find(flagId);
                if (flagNameIter == flagIdNameCache.end()) {
                    flagNameIter = flagIdNameCache.insert(flagId, Flag::retrieveById(flagId).name().toUtf8());
                }
                flags << flagNameIter.value();
                flagQuery.next();
            }
            response.setFlags(flags);
        }

        if (mItemFetchScope.fetchTags() && tagQb) {
            QList<qint64> tagIds;
            QList<Protocol::FetchTagsResponse> tags;
            auto &tagQuery = tagQb->query();
            while (tagQuery.isValid()) {
                PROF_INC(tagsCount)
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

            if (mTagFetchScope.fetchIdOnly()) {
                tags = tagIds | Views::transform([](const auto tagId) {
                           Protocol::FetchTagsResponse resp;
                           resp.setId(tagId);
                           return resp;
                       })
                    | Actions::toQVector;
            } else {
                tags = tagIds | Views::transform([this](const auto tagId) {
                           return HandlerHelper::fetchTagsResponse(Tag::retrieveById(tagId), mTagFetchScope, mConnection);
                       })
                    | Actions::toQVector;
            }
            response.setTags(tags);
        }

        if (mItemFetchScope.fetchVirtualReferences() && vRefQb) {
            QList<qint64> vRefs;
            auto &vRefQuery = vRefQb->query();
            while (vRefQuery.isValid()) {
                PROF_INC(vRefsCount)
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

        if (mItemFetchScope.ancestorDepth() != Protocol::ItemFetchScope::NoAncestor) {
            response.setAncestors(ancestorsForItem(response.parentId()));
        }

        bool skipItem = false;

        QList<QByteArray> cachedParts;
        if (partQb) {
            QList<Protocol::StreamPayloadResponse> parts;
            auto &partQuery = partQb->query();
            while (partQuery.isValid()) {
                PROF_INC(partsCount)
                const qint64 id = partQuery.value(PartQueryPimIdColumn).toLongLong();
                if (id > pimItemId) {
                    partQuery.next();
                    continue;
                } else if (id < pimItemId) {
                    break;
                }

                const qint64 partTypeId = partQuery.value(PartQueryTypeIdColumn).toLongLong();
                auto ptIter = partTypeIdNameCache.find(partTypeId);
                if (ptIter == partTypeIdNameCache.end()) {
                    ptIter = partTypeIdNameCache.insert(partTypeId, PartTypeHelper::fullName(PartType::retrieveById(partTypeId)).toUtf8());
                }
                Protocol::PartMetaData metaPart;
                Protocol::StreamPayloadResponse partData;
                partData.setPayloadName(ptIter.value());
                metaPart.setName(ptIter.value());
                metaPart.setVersion(partQuery.value(PartQueryVersionColumn).toInt());
                metaPart.setSize(partQuery.value(PartQueryDataSizeColumn).toLongLong());

                const QByteArray data = Utils::variantToByteArray(partQuery.value(PartQueryDataColumn));
                if (mItemFetchScope.checkCachedPayloadPartsOnly()) {
                    if (!data.isEmpty()) {
                        cachedParts << ptIter.value();
                    }
                    partQuery.next();
                } else {
                    if (mItemFetchScope.ignoreErrors() && data.isEmpty()) {
                        // We wanted the payload, couldn't get it, and are ignoring errors. Skip the item.
                        // This is not an error though, it's fine to have empty payload parts (to denote existing but not cached parts)
                        qCDebug(AKONADISERVER_LOG) << "item" << id << "has an empty payload part in parttable for part" << metaPart.name();
                        skipItem = true;
                        break;
                    }
                    metaPart.setStorageType(static_cast<Protocol::PartMetaData::StorageType>(partQuery.value(PartQueryStorageColumn).toInt()));
                    if (data.isEmpty()) {
                        partData.setData(QByteArray(""));
                    } else {
                        partData.setData(data);
                    }
                    partData.setMetaData(metaPart);

                    if (mItemFetchScope.requestedParts().contains(ptIter.value()) || mItemFetchScope.fullPayload() || mItemFetchScope.allAttributes()) {
                        parts.append(partData);
                    }

                    partQuery.next();
                }
            }
            response.setParts(parts);
        }

        if (skipItem) {
            itemQuery.next();
            continue;
        }

        if (mItemFetchScope.checkCachedPayloadPartsOnly()) {
            response.setCachedParts(cachedParts);
        }

        if (itemCallback) {
            itemCallback(std::move(response));
        } else {
            mConnection->sendResponse(std::move(response));
        }

        itemQuery.next();
    }
    // Destroy the query builders in order to finalize and cache the prepared statements
    // before doing any more SQL queries.
    tagQb.reset();
    flagQb.reset();
    partQb.reset();
    vRefQb.reset();
    itemQb.reset();
    END_TIMER(processing)

    // update atime (only if the payload was actually requested, otherwise a simple resource sync prevents cache clearing)
    BEGIN_TIMER(aTime)
    if (mUpdateATimeEnabled && (needsAccessTimeUpdate(mItemFetchScope.requestedParts()) || mItemFetchScope.fullPayload())) {
        updateItemAccessTime();
    }
    END_TIMER(aTime)

    END_TIMER(fetch)
#if ENABLE_FETCH_PROFILING
    qCDebug(AKONADISERVER_LOG) << "ItemFetchHelper execution stats:";
    qCDebug(AKONADISERVER_LOG) << "\tItems query:" << itemsElapsed << "ms," << itemsCount << " items in total";
    qCDebug(AKONADISERVER_LOG) << "\tFlags query:" << flagsElapsed << "ms, " << flagsCount << " flags in total";
    qCDebug(AKONADISERVER_LOG) << "\tParts query:" << partsElapsed << "ms, " << partsCount << " parts in total";
    qCDebug(AKONADISERVER_LOG) << "\tTags query: " << tagsElapsed << "ms, " << tagsCount << " tags in total";
    qCDebug(AKONADISERVER_LOG) << "\tVRefs query:" << vRefsElapsed << "ms, " << vRefsCount << " vRefs in total";
    qCDebug(AKONADISERVER_LOG) << "\t------------";
    qCDebug(AKONADISERVER_LOG) << "\tItem retriever:" << itemRetrieverElapsed << "ms (scope local:" << scopeLocalElapsed << "ms)";
    qCDebug(AKONADISERVER_LOG) << "\tTotal query:" << (itemsElapsed + flagsElapsed + partsElapsed + tagsElapsed + vRefsElapsed) << "ms";
    qCDebug(AKONADISERVER_LOG) << "\tTotal processing: " << processingElapsed << "ms";
    qCDebug(AKONADISERVER_LOG) << "\tATime update:" << aTimeElapsed << "ms";
    qCDebug(AKONADISERVER_LOG) << "\t============";
    qCDebug(AKONADISERVER_LOG) << "\tTotal FETCH:" << fetchElapsed << "ms";
    qCDebug(AKONADISERVER_LOG);
    qCDebug(AKONADISERVER_LOG);
#endif

    return true;
}

bool ItemFetchHelper::needsAccessTimeUpdate(const QList<QByteArray> &parts)
{
    // TODO technically we should compare the part list with the cache policy of
    // the parent collection of the retrieved items, but that's kinda expensive
    // Only updating the atime if the full payload was requested is a good
    // approximation though.
    return parts.contains(AKONADI_PARAM_PLD_RFC822);
}

void ItemFetchHelper::updateItemAccessTime()
{
    Transaction transaction(storageBackend(), QStringLiteral("update atime"));
    QueryBuilder qb(PimItem::tableName(), QueryBuilder::Update);
    qb.setColumnValue(PimItem::atimeColumn(), QDateTime::currentDateTimeUtc());
    ItemQueryHelper::scopeToQuery(mScope, mContext, qb);

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Unable to update item access time";
    } else {
        transaction.commit();
    }
}

void ItemFetchHelper::triggerOnDemandFetch()
{
    if (mContext.collectionId() <= 0 || mItemFetchScope.cacheOnly()) {
        return;
    }

    Collection collection = mContext.collection();

    // HACK: don't trigger on-demand syncing if the resource is the one triggering it
    if (mConnection->sessionId() == collection.resource().name().toLatin1()) {
        return;
    }

    storageBackend()->activeCachePolicy(collection);
    if (!collection.cachePolicySyncOnDemand()) {
        return;
    }

    mConnection->akonadi().intervalChecker().requestCollectionSync(collection);
}

QList<Protocol::Ancestor> ItemFetchHelper::ancestorsForItem(Collection::Id parentColId)
{
    if (mItemFetchScope.ancestorDepth() == Protocol::ItemFetchScope::NoAncestor || parentColId == 0) {
        return QList<Protocol::Ancestor>();
    }
    const auto it = mAncestorCache.constFind(parentColId);
    if (it != mAncestorCache.cend()) {
        return *it;
    }

    QList<Protocol::Ancestor> ancestors;
    Collection col = Collection::retrieveById(parentColId);
    const int depthNum = mItemFetchScope.ancestorDepth() == Protocol::ItemFetchScope::ParentAncestor ? 1 : INT_MAX;
    for (int i = 0; i < depthNum; ++i) {
        if (!col.isValid()) {
            Protocol::Ancestor ancestor;
            ancestor.setId(0);
            ancestors << ancestor;
            break;
        }
        Protocol::Ancestor ancestor;
        ancestor.setId(col.id());
        ancestor.setRemoteId(col.remoteId());
        ancestors << ancestor;
        col = col.parent();
    }
    mAncestorCache.insert(parentColId, ancestors);
    return ancestors;
}

QVariant ItemFetchHelper::extractQueryResult(const QSqlQuery &query, ItemFetchHelper::ItemQueryColumns column) const
{
    const int colId = mItemQueryColumnMap[column];
    Q_ASSERT(colId >= 0);
    return query.value(colId);
}
