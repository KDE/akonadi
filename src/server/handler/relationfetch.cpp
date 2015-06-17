/***************************************************************************
 *   Copyright (C) 2014 by Christian Mollekopf <mollekopf@kolabsys.com>    *
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

#include "relationfetch.h"

#include "connection.h"
#include "storage/selectquerybuilder.h"

#include <private/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool RelationFetch::parseStream()
{
    Protocol::FetchRelationsCommand cmd(m_command);

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
    if (!cmd.types().isEmpty()) {
        relationQuery.addJoin(QueryBuilder::InnerJoin, RelationType::tableName(), Relation::typeIdFullColumnName(), RelationType::idFullColumnName());
        relationQuery.addValueCondition(RelationType::nameFullColumnName(), Query::In, cmd.types());
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
        relationQuery.addGroupColumns(QStringList() << Relation::leftIdFullColumnName() << Relation::rightIdFullColumnName() << Relation::typeIdFullColumnName());
    }

    if (!relationQuery.exec()) {
        return failureResponse("Failed to query for existing relation");
    }
    const Relation::List existingRelations = relationQuery.result();
    for (const Relation &relation : existingRelations) {
        Protocol::FetchRelationsResponse response(relation.leftId(), relation.rightId(), relation.relationType().name());
        response.setRemoteId(relation.remoteId());
        sendResponse(response);
    }

    return successResponse<Protocol::FetchRelationsResponse>();
}
