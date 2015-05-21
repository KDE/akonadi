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

#include "relationstore.h"
#include "connection.h"
#include "imapstreamparser.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"
#include "entities.h"
#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool RelationStore::parseStream()
{
    Protocol::ModifyRelationCommand cmd;
    mInStream >> cmd;

    if (cmd.type().isEmpty()) {
        return failureResponse<Protocol::ModifyRelationResponse>(
            QStringLiteral("Relation type not specified"));
    }

    if (cmd.left() < 0 || cmd.right() < 0) {
        return failureResponse<Protocol::ModifyRelationResponse>(
            QStringLiteral("Invalid relation specified"));
    }

    if (!cmd.remoteId().isEmpty() && !connection()->context()->resource().isValid()) {
        return failureResponse<Protocol::ModifyRelationResponse>(
            QStringLiteral("RemoteID can only be set by Resources"));
    }

    RelationType relationType = RelationType::retrieveByName(cmd.type());
    if (!relationType.isValid()) {
        RelationType t(cmd.type());
        if (!t.insert()) {
            return failureResponse<Protocol::ModifyRelationResponse>(
                QStringLiteral("Unable to create relation type '") % cmd.type % QStringLiteral("'"));
        }
        relationType = t;
    }

    SelectQueryBuilder<Relation> relationQuery;
    relationQuery.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, cmd.left());
    relationQuery.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, cmd.right());
    relationQuery.addValueCondition(Relation::typeIdFullColumnName(), Query::Equals, relationType.id());
    if (!relationQuery.exec()) {
        return failureResponse<Protocol::ModifyRelationResponse>(
            QStringLiteral("Failed to query for existing relation"));
    }
    const Relation::List existingRelations = relationQuery.result();
    if (!existingRelations.isEmpty()) {
        if (existingRelations.size() == 1) {
            Relation rel = existingRelations.first();
            if (rel.remoteId() != cmd.remoteId())
            rel.setRemoteId(cmd.remoteId());
            if (!rel.update()) {
                return failureResponse<Protocol::ModifyRelationResponse>(
                    QStringLiteral("Failed to update relation");
            }
        } else {
            return failureResponse<Protocol::ModifyRelationResponse>(
                QStringLiteral("Matched more than one relation"));
        }
        //throw HandlerException("Relation is already existing");
    }

    Relation insertedRelation;
    insertedRelation.setLeftId(cmd.left());
    insertedRelation.setRightId(cmd.right());
    insertedRelation.setTypeId(relationType.id());
    if  (!insertedRelation.insert()) {
        return failureResponse<Protocol::ModifyRelationResponse>(
            QStringLiteral("Failed to store relation"));
    }

    // Get all PIM items that are part of the relation
    SelectQueryBuilder<PimItem> itemsQuery;
    itemsQuery.setSubQueryMode(Query::Or);
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, cmd.left());
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, cmd.right());

    if (!itemsQuery.exec()) {
        return failureResponse<Protocol::ModifyRelationResponse>(
            QStringLiteral("Adding relation failed"));
    }
    const PimItem::List items = itemsQuery.result();

    if (items.size() != 2) {
        return failureResponse<Protocol::ModifyRelationResponse>(
            QStringLiteral("Couldn't find items for relation"));
    }

    /* if (items[0].collection().resourceId() != items[1].collection().resourceId()) {
        throw HandlerException("Relations can only be created for items within the same resource");
    } */

    DataStore::self()->notificationCollector()->relationAdded(insertedRelation);
    DataStore::self()->notificationCollector()->itemsRelationsChanged(items, Relation::List() << insertedRelation, Relation::List());

    mOutStream << Protocol::ModifyRelationResponse();
    return true;
}

