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

#include "collectionqueryhelper.h"

#include "connection.h"
#include "entities.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"
#include "handler.h"
#include "queryhelper.h"

#include <private/scope_p.h>
#include <private/imapset_p.h>

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
        QueryHelper::setToQuery(scope.uidSet(), Collection::idFullColumnName(), qb);
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
    Q_UNUSED(collection);
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
    const QVector<Collection> result = qb.result();
    if (!result.isEmpty()) {
        return result.first().id() == collection.id();
    }
    return true;
}

bool CollectionQueryHelper::canBeMovedTo(const Collection &collection, const Collection &_parent)
{
    if (_parent.isValid()) {
        Collection parent = _parent;
        Q_FOREVER {
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

Collection CollectionQueryHelper::resolveHierarchicalRID(const QVector<Scope::HRID> &ridChain, Resource::Id resId)
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

Collection CollectionQueryHelper::singleCollectionFromScope(const Scope &scope, const CommandContext &context)
{
    // root
    if (scope.scope() == Scope::Uid && scope.uidSet().intervals().count() == 1) {
        const ImapInterval i = scope.uidSet().intervals().at(0);
        if (!i.size()) {   // ### why do we need this hack for 0, shouldn't that be size() == 1?
            Collection root;
            root.setId(0);
            return root;
        }
    }
    SelectQueryBuilder<Collection> qb;
    scopeToQuery(scope, context, qb);
    if (!qb.exec()) {
        throw HandlerException("Unable to execute query");
    }
    const Collection::List cols = qb.result();
    if (cols.isEmpty()) {
        throw HandlerException("No collection found");
    } else if (cols.size() > 1) {
        throw HandlerException("Collection cannot be uniquely identified");
    }
    return cols.first();
}
