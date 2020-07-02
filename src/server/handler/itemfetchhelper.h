/***************************************************************************
 *   SPDX-FileCopyrightText: 2006-2009 Tobias Koenig <tokoe@kde.org>       *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_ITEMFETCHHELPER_H
#define AKONADI_ITEMFETCHHELPER_H


#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"
#include "commandcontext.h"

#include <private/imapset_p.h>
#include <private/scope_p.h>
#include <private/protocol_p.h>

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
    ItemFetchHelper(Connection *connection, const Scope &scope,
            const Protocol::ItemFetchScope &itemFetchScope,
            const Protocol::TagFetchScope &tagFagScope,
            AkonadiServer &akonadi);
    ItemFetchHelper(Connection *connection, const CommandContext &context, const Scope &scope,
            const Protocol::ItemFetchScope &itemFetchScope,
            const Protocol::TagFetchScope &tagFetchScope,
            AkonadiServer &akonadi);

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
    QSqlQuery buildPartQuery(const QVector<QByteArray> &partList, bool allPayload, bool allAttrs);
    QSqlQuery buildFlagQuery();
    QSqlQuery buildTagQuery();
    QSqlQuery buildVRefQuery();

    QVector<Protocol::Ancestor> ancestorsForItem(Collection::Id parentColId);
    static bool needsAccessTimeUpdate(const QVector<QByteArray> &parts);
    QVariant extractQueryResult(const QSqlQuery &query, ItemQueryColumns column) const;
    bool isScopeLocal(const Scope &scope);
    DataStore *storageBackend() const;
    static QByteArray tagsToByteArray(const Tag::List &tags);
    static QByteArray relationsToByteArray(const Relation::List &relations);

private:
    Connection *mConnection = nullptr;
    const CommandContext &mContext;
    QHash<Collection::Id, QVector<Protocol::Ancestor>> mAncestorCache;
    Scope mScope;
    Protocol::ItemFetchScope mItemFetchScope;
    Protocol::TagFetchScope mTagFetchScope;
    int mItemQueryColumnMap[ItemQueryColumnCount];
    bool mUpdateATimeEnabled = true;
    AkonadiServer &mAkonadi;

    friend class ::ItemFetchHelperTest;
};

} // namespace Server
} // namespace Akonadi

#endif
