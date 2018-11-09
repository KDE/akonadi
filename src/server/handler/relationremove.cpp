/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "relationremove.h"

#include "connection.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"
#include "storage/queryhelper.h"
#include "storage/datastore.h"

using namespace Akonadi;
using namespace Akonadi::Server;

bool RelationRemove::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::RemoveRelationsCommand>(m_command);

    if (cmd.left() < 0 || cmd.right() < 0) {
        return failureResponse("Invalid relation id's provided");
    }

    RelationType relType;
    if (!cmd.type().isEmpty()) {
        relType = RelationType::retrieveByName(QString::fromUtf8(cmd.type()));
        if (!relType.isValid()) {
            return failureResponse("Failed to load relation type");
        }
    }

    SelectQueryBuilder<Relation> relationQuery;
    relationQuery.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, cmd.left());
    relationQuery.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, cmd.right());
    if (relType.isValid()) {
        relationQuery.addValueCondition(Relation::typeIdFullColumnName(), Query::Equals, relType.id());
    }

    if (!relationQuery.exec()) {
        return failureResponse("Failed to obtain relations");
    }
    const Relation::List relations = relationQuery.result();
    for (const Relation &relation : relations) {
        storageBackend()->notificationCollector()->relationRemoved(relation);
    }

    // Get all PIM items that are part of the relation
    SelectQueryBuilder<PimItem> itemsQuery;
    itemsQuery.setSubQueryMode(Query::Or);
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, cmd.left());
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, cmd.right());

    if (!itemsQuery.exec()) {
        throw failureResponse("Removing relation failed");
    }
    const PimItem::List items = itemsQuery.result();
    if (!items.isEmpty()) {
        storageBackend()->notificationCollector()->itemsRelationsChanged(items, Relation::List(), relations);
    }

    QueryBuilder qb(Relation::tableName(), QueryBuilder::Delete);
    qb.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, cmd.left());
    qb.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, cmd.right());
    if (relType.isValid()) {
        qb.addValueCondition(Relation::typeIdFullColumnName(), Query::Equals, relType.id());
    }
    if (!qb.exec()) {
        return failureResponse("Failed to remove relations");
    }

    return successResponse<Protocol::RemoveRelationsResponse>();
}

