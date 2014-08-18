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
#include "libs/protocol_p.h"
#include "imapstreamparser.h"

using namespace Akonadi;
using namespace Akonadi::Server;

RelationRemove::RelationRemove(Scope::SelectionScope scope)
    :Handler()
    ,mScope(scope)
{
}

RelationRemove::~RelationRemove()
{
}

bool RelationRemove::parseStream()
{
    QString type;
    qint64 left = -1;
    qint64 right = -1;

    if (mScope.scope() != Scope::Uid) {
        return failureResponse(QByteArray("Only the UID scope is implemented'"));
    }

    while (!m_streamParser->atCommandEnd()) {
        const QByteArray param = m_streamParser->readString();

        if (param == AKONADI_PARAM_LEFT) {
            left = m_streamParser->readNumber();
        } else if (param == AKONADI_PARAM_RIGHT) {
            right = m_streamParser->readNumber();
        } else if (param == AKONADI_PARAM_TYPE) {
            type = m_streamParser->readUtf8String();
        } else {
            return failureResponse(QByteArray("Unknown parameter while removing relation '") + type.toLatin1() + QByteArray("'."));
        }
    }

    if (left < 0 || right < 0) {
        throw HandlerException("Invalid relation id's provided");
    }

    RelationType relType;
    if (!type.isEmpty()) {
        relType = RelationType::retrieveByName(type);
        if (!relType.isValid()) {
            throw HandlerException("Failed to load relation type");
        }
    }

    SelectQueryBuilder<Relation> relationQuery;
    relationQuery.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, left);
    relationQuery.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, right);
    if (relType.isValid()) {
        relationQuery.addValueCondition(Relation::typeIdFullColumnName(), Query::Equals, relType.id());
    }

    if (!relationQuery.exec()) {
        throw HandlerException("Failed to obtain relations");
    }
    const Relation::List relations = relationQuery.result();
    Q_FOREACH (const Relation &relation, relations) {
        DataStore::self()->notificationCollector()->relationRemoved(relation);
    }

    // Get all PIM items that that are part of the relation
    SelectQueryBuilder<PimItem> itemsQuery;
    itemsQuery.setSubQueryMode(Query::Or);
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, left);
    itemsQuery.addValueCondition(PimItem::idColumn(), Query::Equals, right);

    if (!itemsQuery.exec()) {
        throw HandlerException("Removing relation failed");
    }
    const PimItem::List items = itemsQuery.result();

    if (!items.isEmpty()) {
        DataStore::self()->notificationCollector()->itemsRelationsChanged(items, Relation::List(), relations);
    }

    QueryBuilder qb(Relation::tableName(), QueryBuilder::Delete);
    qb.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, left);
    qb.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, right);
    if (relType.isValid()) {
        qb.addValueCondition(Relation::typeIdFullColumnName(), Query::Equals, relType.id());
    }
    if (!qb.exec()) {
        throw HandlerException("Deletion failed");
    }

    return successResponse("RELATIONREMOVE complete");
}

