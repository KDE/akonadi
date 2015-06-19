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

#include "itemqueryhelper.h"

#include "commandcontext.h"
#include "entities.h"
#include "storage/querybuilder.h"
#include "handler.h"
#include "storage/queryhelper.h"
#include "collectionqueryhelper.h"

#include <private/scope_p.h>
#include <private/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

void ItemQueryHelper::itemSetToQuery(const ImapSet &set, QueryBuilder &qb, const Collection &collection)
{
    QueryHelper::setToQuery(set, PimItem::idFullColumnName(), qb);
    if (collection.isValid()) {
        if (collection.isVirtual() || collection.resource().isVirtual()) {
            qb.addJoin(QueryBuilder::InnerJoin, CollectionPimItemRelation::tableName(),
                       CollectionPimItemRelation::rightFullColumnName(), PimItem::idFullColumnName());
            qb.addValueCondition(CollectionPimItemRelation::leftFullColumnName(), Query::Equals, collection.id());
        } else {
            qb.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, collection.id());
        }
    }
}

void ItemQueryHelper::itemSetToQuery(const ImapSet &set, CommandContext *context, QueryBuilder &qb)
{
    if (context->collectionId() >= 0) {
        itemSetToQuery(set, qb, context->collection());
    } else {
        itemSetToQuery(set, qb);
    }

    if (context->tagId() >= 0) {
        //When querying for items by tag, only return matches from that resource
        if (context->resource().isValid()) {
            qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(),
                        PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
            qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, context->resource().id());
        }
        qb.addJoin(QueryBuilder::InnerJoin, PimItemTagRelation::tableName(),
                   PimItem::idFullColumnName(), PimItemTagRelation::leftFullColumnName());
        qb.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::Equals, context->tagId());
    }
}

void ItemQueryHelper::remoteIdToQuery(const QStringList &rids, CommandContext *context, QueryBuilder &qb)
{
    if (rids.size() == 1) {
        qb.addValueCondition(PimItem::remoteIdFullColumnName(), Query::Equals, rids.first());
    } else {
        qb.addValueCondition(PimItem::remoteIdFullColumnName(), Query::In, rids);
    }

    if (context->resource().isValid()) {
        qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(),
                   PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
        qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, context->resource().id());
    }
    if (context->collectionId() > 0) {
        qb.addValueCondition(PimItem::collectionIdFullColumnName(), Query::Equals, context->collectionId());
    }
    if (context->tagId() > 0) {
        qb.addJoin(QueryBuilder::InnerJoin, PimItemTagRelation::tableName(),
                   PimItem::idFullColumnName(), PimItemTagRelation::leftFullColumnName());
        qb.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::Equals, context->tagId());
    }
}

void ItemQueryHelper::gidToQuery(const QStringList &gids, CommandContext *context, QueryBuilder &qb)
{
    if (gids.size() == 1) {
        qb.addValueCondition(PimItem::gidFullColumnName(), Query::Equals, gids.first());
    } else {
        qb.addValueCondition(PimItem::gidFullColumnName(), Query::In, gids);
    }

    if (context->tagId() > 0) {
        //When querying for items by tag, only return matches from that resource
        if (context->resource().isValid()) {
            qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(),
                        PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
            qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, context->resource().id());
        }
        qb.addJoin(QueryBuilder::InnerJoin, PimItemTagRelation::tableName(),
                   PimItem::idFullColumnName(), PimItemTagRelation::leftFullColumnName());
        qb.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::Equals, context->tagId());
    }
}

void ItemQueryHelper::scopeToQuery(const Scope &scope, CommandContext *context, QueryBuilder &qb)
{
    if (scope.scope() == Scope::Uid) {
        itemSetToQuery(scope.uidSet(), context, qb);
        return;
    }

    if (scope.scope() == Scope::Gid) {
        ItemQueryHelper::gidToQuery(scope.gidSet(), context, qb);
        return;
    }

    if (context->collectionId() <= 0 && !context->resource().isValid()) {
        throw HandlerException("Operations based on remote identifiers require a resource or collection context");
    }

    if (scope.scope() == Scope::Rid) {
        ItemQueryHelper::remoteIdToQuery(scope.ridSet(), context, qb);
        return;
    } else if (scope.scope() == Scope::HierarchicalRid) {
        QVector<Scope::HRID> hridChain = scope.hridChain();
        const Scope::HRID itemHRID = hridChain.takeFirst();
        const Collection parentCol = CollectionQueryHelper::resolveHierarchicalRID(hridChain, context->resource().id());
        const Collection oldSelection = context->collection();
        context->setCollection(parentCol);
        remoteIdToQuery(QStringList() << itemHRID.remoteId, context, qb);
        context->setCollection(oldSelection);
        return;
    }

    throw HandlerException("Dude, WTF?!?");
}
