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

using namespace Akonadi::Server;

RelationStore::RelationStore(Scope::SelectionScope scope)
    :Handler()
    ,mScope(scope)
{
}

RelationStore::~RelationStore()
{
}

Relation RelationStore::fetchRelation(qint64 leftId, qint64 rightId, qint64 typeId)
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


bool RelationStore::parseStream()
{
    QString type;
    QString remoteId;
    qint64 left = -1;
    qint64 right = -1;

    if (mScope.scope() != Scope::Uid) {
        return failureResponse(QByteArray("Only uid based parents are supported.'"));
    }

    while (!m_streamParser->atCommandEnd()) {
        const QByteArray param = m_streamParser->readString();

        if (param == AKONADI_PARAM_LEFT) {
            if (mScope.scope() == Scope::Uid) {
                left = m_streamParser->readNumber();
            }
        } else if (param == AKONADI_PARAM_RIGHT) {
            if (mScope.scope() == Scope::Uid) {
                right = m_streamParser->readNumber();
            }
        } else if (param == AKONADI_PARAM_TYPE) {
            type = m_streamParser->readUtf8String();
        } else if (param == AKONADI_PARAM_REMOTEID) {
            if (!connection()->context()->resource().isValid()) {
                throw HandlerException("remote id can only be set in resource context");
            }
            remoteId = m_streamParser->readUtf8String();
        } else {
            return failureResponse(QByteArray("Unknown parameter while creating relation '") + type.toLatin1() + QByteArray("'."));
        }
    }

    RelationType relationType = RelationType::retrieveByName(type);
    if (!relationType.isValid()) {
        RelationType t(type);
        if (!t.insert()) {
            return failureResponse(QByteArray("Unable to create relationtype '") + type.toLatin1() + QByteArray("'."));
        }
        relationType = t;
    }

    Relation existingRelation = fetchRelation(left, right, relationType.id());
    if (existingRelation.isValid()) {
        existingRelation.setRemoteId(remoteId);
        if (!existingRelation.update()) {
            throw HandlerException("Failed to update relation");
        }
        // FIXME: Are we actually throwing exception for a valid case?
        throw HandlerException("Relation already exists");
    }

    // Can't use insert(), does not work here (no "id" column)
    QueryBuilder inQb(Relation::tableName(), QueryBuilder::Insert);
    inQb.setIdentificationColumn(QString()); // omit "RETURING xyz" with PSQL
    inQb.setColumnValue(Relation::leftIdColumn(), left);
    inQb.setColumnValue(Relation::rightIdColumn(), right);
    inQb.setColumnValue(Relation::typeIdColumn(), relationType.id());
    if (!inQb.exec()) {
        throw HandlerException("Failed to store relation");
    }

    Relation insertedRelation = fetchRelation(left, right, relationType.id());

    // Get all PIM items that are part of the relation
    SelectQueryBuilder<PimItem> itemsQuery;
    itemsQuery.setSubQueryMode(Query::Or);
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, left);
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, right);

    if (!itemsQuery.exec()) {
        throw HandlerException("Adding relation failed");
    }
    const PimItem::List items = itemsQuery.result();

    if (items.size() != 2) {
        throw HandlerException("Couldn't find items for relation");
    }

    /* if (items[0].collection().resourceId() != items[1].collection().resourceId()) {
        throw HandlerException("Relations can only be created for items within the same resource");
    } */

    DataStore::self()->notificationCollector()->relationAdded(insertedRelation);
    DataStore::self()->notificationCollector()->itemsRelationsChanged(items, Relation::List() << insertedRelation, Relation::List());

    Response response;
    response.setTag(tag());
    response.setSuccess();
    response.setString("Store completed");
    Q_EMIT responseAvailable(response);
    return true;
}

