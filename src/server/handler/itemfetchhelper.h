/***************************************************************************
 *   SPDX-FileCopyrightText: 2006-2009 Tobias Koenig <tokoe@kde.org>       *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include "commandcontext.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"

#include <private/imapset_p.h>
#include <private/protocol_p.h>
#include <private/scope_p.h>

#include <functional>

class ItemFetchHelperTest;

namespace Akonadi
{
namespace Server
{
class Connection;
class AkonadiServer;

class ItemFetchHelper
{
public:
    ItemFetchHelper(Connection *connection,
                    const Scope &scope,
                    const Protocol::ItemFetchScope &itemFetchScope,
                    const Protocol::TagFetchScope &tagFagScope,
                    AkonadiServer &akonadi,
                    const Protocol::FetchLimit &itemsLimit = Protocol::FetchLimit());
    ItemFetchHelper(Connection *connection,
                    const CommandContext &context,
                    const Scope &scope,
                    const Protocol::ItemFetchScope &itemFetchScope,
                    const Protocol::TagFetchScope &tagFetchScope,
                    AkonadiServer &akonadi,
                    const Protocol::FetchLimit &itemsLimit = Protocol::FetchLimit());

    bool fetchItems(std::function<void(Protocol::FetchItemsResponse &&)> &&callback = {});

    void disableATimeUpdates();

private:
    enum ItemQueryColumns {
        ItemQueryPimItemIdColumn,
        ItemQueryPimItemRidColumn,
        ItemQueryMimeTypeIdColumn,
        ItemQueryRevColumn,
        ItemQueryRemoteRevisionColumn,
        ItemQuerySizeColumn,
        ItemQueryDatetimeColumn,
        ItemQueryCollectionIdColumn,
        ItemQueryPimItemGidColumn,
        ItemQueryColumnCount
    };

    void updateItemAccessTime();
    void triggerOnDemandFetch();
    QSqlQuery buildItemQuery();
    QSqlQuery buildPartQuery(const QList<QByteArray> &partList, bool allPayload, bool allAttrs);
    QSqlQuery buildFlagQuery();
    QSqlQuery buildTagQuery();
    QSqlQuery buildVRefQuery();

    QList<Protocol::Ancestor> ancestorsForItem(Collection::Id parentColId);
    static bool needsAccessTimeUpdate(const QList<QByteArray> &parts);
    QVariant extractQueryResult(const QSqlQuery &query, ItemQueryColumns column) const;
    bool isScopeLocal(const Scope &scope);
    DataStore *storageBackend() const;

private:
    Connection *mConnection = nullptr;
    const CommandContext &mContext;
    QHash<Collection::Id, QList<Protocol::Ancestor>> mAncestorCache;
    Scope mScope;
    Protocol::ItemFetchScope mItemFetchScope;
    Protocol::TagFetchScope mTagFetchScope;
    int mItemQueryColumnMap[ItemQueryColumnCount];
    bool mUpdateATimeEnabled = true;
    AkonadiServer &mAkonadi;
    Protocol::FetchLimit mItemsLimit;
    QueryBuilder mItemQuery;
    QString mPimItemQueryAlias;

    friend class ::ItemFetchHelperTest;
};

} // namespace Server
} // namespace Akonadi
