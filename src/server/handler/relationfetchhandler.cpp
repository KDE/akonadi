/***************************************************************************
 *   SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>    *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "relationfetchhandler.h"
#include "connection.h"
#include "handlerhelper.h"
#include "storage/selectquerybuilder.h"

#include <private/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

RelationFetchHandler::RelationFetchHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool RelationFetchHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::FetchRelationsCommand>(m_command);

    SelectQueryBuilder<Relation> relationQuery;
    if (cmd.side() > 0) {
        Query::Condition c;
        c.setSubQueryMode(Query::Or);
        c.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, cmd.side());
        c.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, cmd.side());
        relationQuery.addCondition(c);
    } else {
        if (cmd.left() > 0) {
            relationQuery.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, cmd.left());
        }
        if (cmd.right() > 0) {
            relationQuery.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, cmd.right());
        }
    }

    const auto cmdTypes = cmd.types();
    if (!cmdTypes.isEmpty()) {
        relationQuery.addJoin(QueryBuilder::InnerJoin, RelationType::tableName(), Relation::typeIdFullColumnName(), RelationType::idFullColumnName());
        QStringList types;
        types.reserve(cmdTypes.size());
        for (const QByteArray &type : cmdTypes) {
            types << QString::fromUtf8(type);
        }
        relationQuery.addValueCondition(RelationType::nameFullColumnName(), Query::In, types);
    }
    if (!cmd.resource().isEmpty()) {
        Resource res = Resource::retrieveByName(cmd.resource());
        if (!res.isValid()) {
            return failureResponse("Invalid resource");
        }
        Query::Condition condition;
        condition.setSubQueryMode(Query::Or);
        condition.addColumnCondition(PimItem::idFullColumnName(), Query::Equals, Relation::leftIdFullColumnName());
        condition.addColumnCondition(PimItem::idFullColumnName(), Query::Equals, Relation::rightIdFullColumnName());
        relationQuery.addJoin(QueryBuilder::InnerJoin, PimItem::tableName(), condition);

        relationQuery.addJoin(QueryBuilder::InnerJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());

        relationQuery.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, res.id());
        relationQuery.addGroupColumns(QStringList() << Relation::leftIdFullColumnName() << Relation::rightIdFullColumnName()
                                                    << Relation::typeIdFullColumnName());
    }

    if (!relationQuery.exec()) {
        return failureResponse("Failed to query for existing relation");
    }
    const Relation::List existingRelations = relationQuery.result();
    for (const Relation &relation : existingRelations) {
        sendResponse(HandlerHelper::fetchRelationsResponse(relation));
    }

    return successResponse<Protocol::FetchRelationsResponse>();
}
