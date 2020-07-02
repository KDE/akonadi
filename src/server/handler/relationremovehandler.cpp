/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "relationremovehandler.h"

#include "connection.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"
#include "storage/queryhelper.h"
#include "storage/datastore.h"

using namespace Akonadi;
using namespace Akonadi::Server;

RelationRemoveHandler::RelationRemoveHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

bool RelationRemoveHandler::parseStream()
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

