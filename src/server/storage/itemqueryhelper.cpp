/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemqueryhelper.h"

#include "collectionqueryhelper.h"
#include "commandcontext.h"
#include "handler.h"
#include "storage/querybuilder.h"
#include "storage/queryhelper.h"

#include "private/imapset_p.h"
#include "private/scope_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

void ItemQueryHelper::itemSetToQuery(const ImapSet &set, QueryBuilder &qb, const Collection &collection)
{
    if (!set.isEmpty()) {
        QueryHelper::setToQuery(set, qb.getTableWithColumn(PimItem::idColumn()), qb);
    }
    if (collection.isValid()) {
        if (collection.isVirtual() || collection.resource().isVirtual()) {
            qb.addJoin(QueryBuilder::InnerJoin,
                       CollectionPimItemRelation::tableName(),
                       CollectionPimItemRelation::rightFullColumnName(),
                       qb.getTableWithColumn(PimItem::idColumn()));
            qb.addValueCondition(CollectionPimItemRelation::leftFullColumnName(), Query::Equals, collection.id());
        } else {
            qb.addValueCondition(qb.getTableWithColumn(PimItem::collectionIdColumn()), Query::Equals, collection.id());
        }
    }
}

void ItemQueryHelper::itemSetToQuery(const ImapSet &set, const CommandContext &context, QueryBuilder &qb)
{
    if (context.collectionId() >= 0) {
        itemSetToQuery(set, qb, context.collection());
    } else {
        itemSetToQuery(set, qb);
    }

    const auto tagId = context.tagId();
    if (tagId.has_value()) {
        // When querying for items by tag, only return matches from that resource
        if (context.resource().isValid()) {
            qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(), qb.getTableWithColumn(PimItem::collectionIdColumn()), Collection::idFullColumnName());
            qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, context.resource().id());
        }
        qb.addJoin(QueryBuilder::InnerJoin,
                   PimItemTagRelation::tableName(),
                   qb.getTableWithColumn(PimItem::idColumn()),
                   PimItemTagRelation::leftFullColumnName());
        qb.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::Equals, *tagId);
    }
}

void ItemQueryHelper::remoteIdToQuery(const QStringList &rids, const CommandContext &context, QueryBuilder &qb)
{
    if (rids.size() == 1) {
        qb.addValueCondition(qb.getTableWithColumn(PimItem::remoteIdColumn()), Query::Equals, rids.first());
    } else {
        qb.addValueCondition(qb.getTableWithColumn(PimItem::remoteIdColumn()), Query::In, rids);
    }

    if (context.resource().isValid()) {
        qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(), qb.getTableWithColumn(PimItem::collectionIdColumn()), Collection::idFullColumnName());
        qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, context.resource().id());
    }
    if (context.collectionId() > 0) {
        qb.addValueCondition(qb.getTableWithColumn(PimItem::collectionIdColumn()), Query::Equals, context.collectionId());
    }

    const auto tagId = context.tagId();
    if (tagId.has_value()) {
        qb.addJoin(QueryBuilder::InnerJoin,
                   PimItemTagRelation::tableName(),
                   qb.getTableWithColumn(PimItem::idColumn()),
                   PimItemTagRelation::leftFullColumnName());
        qb.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::Equals, *tagId);
    }
}

void ItemQueryHelper::gidToQuery(const QStringList &gids, const CommandContext &context, QueryBuilder &qb)
{
    if (gids.size() == 1) {
        qb.addValueCondition(qb.getTableWithColumn(PimItem::gidColumn()), Query::Equals, gids.first());
    } else {
        qb.addValueCondition(qb.getTableWithColumn(PimItem::gidColumn()), Query::In, gids);
    }

    const auto tagId = context.tagId();
    if (tagId.has_value()) {
        // When querying for items by tag, only return matches from that resource
        if (context.resource().isValid()) {
            qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(), qb.getTableWithColumn(PimItem::collectionIdColumn()), Collection::idFullColumnName());
            qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, context.resource().id());
        }
        qb.addJoin(QueryBuilder::InnerJoin,
                   PimItemTagRelation::tableName(),
                   qb.getTableWithColumn(PimItem::idColumn()),
                   PimItemTagRelation::leftFullColumnName());
        qb.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::Equals, *tagId);
    }
}

void ItemQueryHelper::scopeToQuery(const Scope &scope, const CommandContext &context, QueryBuilder &qb)
{
    // Handle fetch by collection/tag
    if (scope.scope() == Scope::Invalid && !context.isEmpty()) {
        itemSetToQuery(ImapSet(), context, qb);
        return;
    }

    if (scope.scope() == Scope::Uid) {
        itemSetToQuery(scope.uidSet(), context, qb);
        return;
    }

    if (scope.scope() == Scope::Gid) {
        ItemQueryHelper::gidToQuery(scope.gidSet(), context, qb);
        return;
    }

    if (context.collectionId() <= 0 && !context.resource().isValid()) {
        throw HandlerException("Operations based on remote identifiers require a resource or collection context");
    }

    if (scope.scope() == Scope::Rid) {
        ItemQueryHelper::remoteIdToQuery(scope.ridSet(), context, qb);
        return;
    } else if (scope.scope() == Scope::HierarchicalRid) {
        QList<Scope::HRID> hridChain = scope.hridChain();
        const Scope::HRID itemHRID = hridChain.takeFirst();
        const Collection parentCol = CollectionQueryHelper::resolveHierarchicalRID(hridChain, context.resource().id());
        const Collection oldSelection = context.collection();
        CommandContext tmpContext(context);
        tmpContext.setCollection(parentCol);
        remoteIdToQuery(QStringList() << itemHRID.remoteId, tmpContext, qb);
        return;
    }

    throw HandlerException("Dude, WTF?!?");
}
