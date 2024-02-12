/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionqueryhelper.h"

#include "handler.h"
#include "storage/selectquerybuilder.h"

using namespace Akonadi;
using namespace Akonadi::Server;

void CollectionQueryHelper::remoteIdToQuery(const QStringList &rids, const CommandContext &context, QueryBuilder &qb)
{
    if (rids.size() == 1) {
        qb.addValueCondition(Collection::remoteIdFullColumnName(), Query::Equals, rids.first());
    } else {
        qb.addValueCondition(Collection::remoteIdFullColumnName(), Query::In, rids);
    }

    if (context.resource().isValid()) {
        qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, context.resource().id());
    }
}

void CollectionQueryHelper::scopeToQuery(const Scope &scope, const CommandContext &context, QueryBuilder &qb)
{
    if (scope.scope() == Scope::Uid) {
        if (!scope.isEmpty()) {
            qb.addValueCondition(Collection::idFullColumnName(), Query::In, scope.uidSet());
        }
    } else if (scope.scope() == Scope::Rid) {
        if (context.collectionId() <= 0 && !context.resource().isValid()) {
            throw HandlerException("Operations based on remote identifiers require a resource or collection context");
        }
        CollectionQueryHelper::remoteIdToQuery(scope.ridSet(), context, qb);
    } else if (scope.scope() == Scope::HierarchicalRid) {
        if (!context.resource().isValid()) {
            throw HandlerException("Operations based on hierarchical remote identifiers require a resource or collection context");
        }
        const Collection c = CollectionQueryHelper::resolveHierarchicalRID(scope.hridChain(), context.resource().id());
        qb.addValueCondition(Collection::idFullColumnName(), Query::Equals, c.id());
    } else {
        throw HandlerException("WTF?");
    }
}

bool CollectionQueryHelper::hasAllowedName(const Collection &collection, const QString &name, Collection::Id parent)
{
    Q_UNUSED(collection)
    SelectQueryBuilder<Collection> qb;
    if (parent > 0) {
        qb.addValueCondition(Collection::parentIdColumn(), Query::Equals, parent);
    } else {
        qb.addValueCondition(Collection::parentIdColumn(), Query::Is, QVariant());
    }
    qb.addValueCondition(Collection::nameColumn(), Query::Equals, name);
    if (!qb.exec()) {
        return false;
    }
    const QList<Collection> result = qb.result();
    if (!result.isEmpty()) {
        return result.first().id() == collection.id();
    }
    return true;
}

bool CollectionQueryHelper::canBeMovedTo(const Collection &collection, const Collection &_parent)
{
    if (_parent.isValid()) {
        Collection parent = _parent;
        for (;;) {
            if (parent.id() == collection.id()) {
                return false; // target is child of source
            }
            if (parent.parentId() == 0) {
                break;
            }
            parent = parent.parent();
        }
    }
    return hasAllowedName(collection, collection.name(), _parent.id());
}

Collection CollectionQueryHelper::resolveHierarchicalRID(const QList<Scope::HRID> &ridChain, Resource::Id resId)
{
    if (ridChain.size() < 2) {
        throw HandlerException("Empty or incomplete hierarchical RID chain");
    }
    if (!ridChain.last().isEmpty()) {
        throw HandlerException("Hierarchical RID chain is not root-terminated");
    }
    Collection::Id parentId = 0;
    Collection result;
    for (int i = ridChain.size() - 2; i >= 0; --i) {
        SelectQueryBuilder<Collection> qb;
        if (parentId > 0) {
            qb.addValueCondition(Collection::parentIdColumn(), Query::Equals, parentId);
        } else {
            qb.addValueCondition(Collection::parentIdColumn(), Query::Is, QVariant());
        }
        qb.addValueCondition(Collection::remoteIdColumn(), Query::Equals, ridChain.at(i).remoteId);
        qb.addValueCondition(Collection::resourceIdColumn(), Query::Equals, resId);
        if (!qb.exec()) {
            throw HandlerException("Unable to execute query");
        }
        const Collection::List results = qb.result();
        const int resultSize = results.size();
        if (resultSize == 0) {
            throw HandlerException("Hierarchical RID does not specify an existing collection");
        } else if (resultSize > 1) {
            throw HandlerException("Hierarchical RID does not specify a unique collection");
        }
        result = results.first();
        parentId = result.id();
    }
    return result;
}
