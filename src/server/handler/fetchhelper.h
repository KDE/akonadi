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

#ifndef AKONADI_FETCHHELPER_H
#define AKONADI_FETCHHELPER_H


#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"
#include "commandcontext.h"

#include <private/imapset_p.h>
#include <private/scope_p.h>
#include <private/protocol_p.h>

#include <functional>

class FetchHelperTest;

namespace Akonadi
{

namespace Server
{

class AggregatedItemFetchScope;
class Connection;

class FetchHelper
{
public:
    FetchHelper(Connection *connection, const Scope &scope, const Protocol::ItemFetchScope &fetchScope);
    FetchHelper(Connection *connection, CommandContext *context, const Scope &scope, const Protocol::ItemFetchScope &fetchScope);

    bool fetchItems(std::function<void(Protocol::FetchItemsResponse &&)> &&callback = {});

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
    CommandContext *mContext = nullptr;
    QHash<Collection::Id, QVector<Protocol::Ancestor>> mAncestorCache;
    Scope mScope;
    Protocol::ItemFetchScope mFetchScope;
    int mItemQueryColumnMap[ItemQueryColumnCount];

    friend class ::FetchHelperTest;
};

} // namespace Server
} // namespace Akonadi

#endif
