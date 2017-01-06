/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2015 Daniel Vrátil <dvratil@redhat.com>

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

#include "tagqueryhelper.h"

#include "commandcontext.h"
#include "storage/querybuilder.h"
#include "handler.h"
#include "storage/queryhelper.h"

#include <private/scope_p.h>
#include <private/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

void TagQueryHelper::remoteIdToQuery(const QStringList &rids, CommandContext *context, QueryBuilder &qb)
{
    qb.addJoin(QueryBuilder::InnerJoin, TagRemoteIdResourceRelation::tableName(),
               Tag::idFullColumnName(), TagRemoteIdResourceRelation::tagIdFullColumnName());
    qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(), Query::Equals, context->resource().id());

    if (rids.size() == 1) {
        qb.addValueCondition(TagRemoteIdResourceRelation::remoteIdFullColumnName(), Query::Equals, rids.first());
    } else {
        qb.addValueCondition(TagRemoteIdResourceRelation::remoteIdFullColumnName(), Query::In, rids);
    }
}

void TagQueryHelper::gidToQuery(const QStringList &gids, CommandContext *context, QueryBuilder &qb)
{
    if (context->resource().isValid()) {
        qb.addJoin(QueryBuilder::InnerJoin, TagRemoteIdResourceRelation::tableName(),
                   Tag::idFullColumnName(), TagRemoteIdResourceRelation::tagIdFullColumnName());
        qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(), Query::Equals, context->resource().id());
    }

    if (gids.size() == 1) {
        qb.addValueCondition(Tag::gidFullColumnName(), Query::Equals, gids.first());
    } else {
        qb.addValueCondition(Tag::gidFullColumnName(), Query::In, gids);
    }
}

void TagQueryHelper::scopeToQuery(const Scope &scope, CommandContext *context, QueryBuilder &qb)
{
    if (scope.scope() == Scope::Uid) {
        QueryHelper::setToQuery(scope.uidSet(), Tag::idFullColumnName(), qb);
        return;
    }

    if (scope.scope() == Scope::Gid) {
        TagQueryHelper::gidToQuery(scope.gidSet(), context, qb);
        return;
    }

    if (scope.scope() == Scope::Rid) {
        if (!context->resource().isValid()) {
            throw HandlerException("Operations based on remote identifiers require a resource or collection context");
        }

        if (scope.scope() == Scope::Rid) {
            TagQueryHelper::remoteIdToQuery(scope.ridSet(), context, qb);
        }
    }

    throw HandlerException("HRID tag operations are not permitted");
}
