/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagqueryhelper.h"

#include "commandcontext.h"
#include "handler.h"
#include "storage/querybuilder.h"

#include "private/scope_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

void TagQueryHelper::remoteIdToQuery(const QStringList &rids, const CommandContext &context, QueryBuilder &qb)
{
    qb.addJoin(QueryBuilder::InnerJoin, TagRemoteIdResourceRelation::tableName(), Tag::idFullColumnName(), TagRemoteIdResourceRelation::tagIdFullColumnName());
    qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(), Query::Equals, context.resource().id());

    if (rids.size() == 1) {
        qb.addValueCondition(TagRemoteIdResourceRelation::remoteIdFullColumnName(), Query::Equals, rids.first());
    } else {
        qb.addValueCondition(TagRemoteIdResourceRelation::remoteIdFullColumnName(), Query::In, rids);
    }
}

void TagQueryHelper::gidToQuery(const QStringList &gids, const CommandContext &context, QueryBuilder &qb)
{
    if (context.resource().isValid()) {
        qb.addJoin(QueryBuilder::InnerJoin,
                   TagRemoteIdResourceRelation::tableName(),
                   Tag::idFullColumnName(),
                   TagRemoteIdResourceRelation::tagIdFullColumnName());
        qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(), Query::Equals, context.resource().id());
    }

    if (gids.size() == 1) {
        qb.addValueCondition(Tag::gidFullColumnName(), Query::Equals, gids.first());
    } else {
        qb.addValueCondition(Tag::gidFullColumnName(), Query::In, gids);
    }
}

void TagQueryHelper::scopeToQuery(const Scope &scope, const CommandContext &context, QueryBuilder &qb)
{
    if (scope.scope() == Scope::Invalid) {
        return;
    }

    if (scope.scope() == Scope::Uid) {
        if (!scope.isEmpty()) {
            qb.addValueCondition(Tag::idFullColumnName(), Query::In, scope.uidSet());
        }
        return;
    }

    if (scope.scope() == Scope::Gid) {
        TagQueryHelper::gidToQuery(scope.gidSet(), context, qb);
        return;
    }

    if (scope.scope() == Scope::Rid) {
        if (!context.resource().isValid()) {
            throw HandlerException("Operations based on remote identifiers require a resource or collection context");
        }

        if (scope.scope() == Scope::Rid) {
            TagQueryHelper::remoteIdToQuery(scope.ridSet(), context, qb);
        }
    }

    throw HandlerException("HRID tag operations are not permitted");
}
