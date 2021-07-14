/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "relationmodifyhandler.h"

#include "connection.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"

using namespace Akonadi;
using namespace Akonadi::Server;

RelationModifyHandler::RelationModifyHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

Relation RelationModifyHandler::fetchRelation(qint64 leftId, qint64 rightId, qint64 typeId)
{
    SelectQueryBuilder<Relation> relationQuery;
    relationQuery.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, leftId);
    relationQuery.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, rightId);
    relationQuery.addValueCondition(Relation::typeIdFullColumnName(), Query::Equals, typeId);
    if (!relationQuery.exec()) {
        throw HandlerException("Failed to query for existing relation");
    }
    const Relation::List existingRelations = relationQuery.result();
    if (!existingRelations.isEmpty()) {
        if (existingRelations.size() == 1) {
            return existingRelations.at(0);
        } else {
            throw HandlerException("Matched more than 1 relation");
        }
    }

    return Relation();
}

bool RelationModifyHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::ModifyRelationCommand>(m_command);

    if (cmd.type().isEmpty()) {
        return failureResponse("Relation type not specified");
    }

    if (cmd.left() < 0 || cmd.right() < 0) {
        return failureResponse("Invalid relation specified");
    }

    if (!cmd.remoteId().isEmpty() && !connection()->context().resource().isValid()) {
        return failureResponse("RemoteID can only be set by Resources");
    }

    const QString typeName = QString::fromUtf8(cmd.type());
    const RelationType relationType = RelationType::retrieveByNameOrCreate(typeName);
    if (!relationType.isValid()) {
        return failureResponse(QStringLiteral("Unable to create relation type '") % typeName % QStringLiteral("'"));
    }

    Relation existingRelation = fetchRelation(cmd.left(), cmd.right(), relationType.id());
    if (existingRelation.isValid()) {
        existingRelation.setRemoteId(QLatin1String(cmd.remoteId()));
        if (!existingRelation.update()) {
            return failureResponse("Failed to update relation");
        }
    }

    // Can't use insert(), does not work here (no "id" column)
    QueryBuilder inQb(Relation::tableName(), QueryBuilder::Insert);
    inQb.setIdentificationColumn(QString()); // omit "RETURNING xyz" with PSQL
    inQb.setColumnValue(Relation::leftIdColumn(), cmd.left());
    inQb.setColumnValue(Relation::rightIdColumn(), cmd.right());
    inQb.setColumnValue(Relation::typeIdColumn(), relationType.id());
    if (!inQb.exec()) {
        throw HandlerException("Failed to store relation");
    }

    Relation insertedRelation = fetchRelation(cmd.left(), cmd.right(), relationType.id());

    // Get all PIM items that are part of the relation
    SelectQueryBuilder<PimItem> itemsQuery;
    itemsQuery.setSubQueryMode(Query::Or);
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, cmd.left());
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, cmd.right());

    if (!itemsQuery.exec()) {
        return failureResponse("Adding relation failed");
    }
    const PimItem::List items = itemsQuery.result();

    if (items.size() != 2) {
        return failureResponse("Couldn't find items for relation");
    }

    /* if (items[0].collection().resourceId() != items[1].collection().resourceId()) {
        throw HandlerException("Relations can only be created for items within the same resource");
    } */

    auto collector = storageBackend()->notificationCollector();
    collector->relationAdded(insertedRelation);
    collector->itemsRelationsChanged(items, {insertedRelation}, {});

    return successResponse<Protocol::ModifyRelationResponse>();
}
