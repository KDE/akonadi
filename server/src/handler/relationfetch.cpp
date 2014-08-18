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
#include "imapstreamparser.h"
#include "connection.h"
#include "response.h"
#include "libs/imapset_p.h"
#include "libs/imapparser_p.h"
#include "libs/protocol_p.h"
#include "storage/selectquerybuilder.h"

using namespace Akonadi;
using namespace Akonadi::Server;

RelationFetch::RelationFetch(Scope::SelectionScope scope)
    : Handler()
    , mScope(scope)
{
}

RelationFetch::~RelationFetch()
{
}

QByteArray RelationFetch::relationToByteArray(qint64 leftId, qint64 rightId, const QByteArray &type, const QByteArray &rid)
{
    QList<QByteArray> attributes;
    attributes << AKONADI_PARAM_LEFT << QByteArray::number(leftId);
    attributes << AKONADI_PARAM_RIGHT << QByteArray::number(rightId);
    attributes << AKONADI_PARAM_TYPE << type;
    if (!rid.isEmpty()) {
        attributes << AKONADI_PARAM_REMOTEID << rid;
    }

    return ImapParser::join(attributes, " ");
}

bool RelationFetch::parseStream()
{
    QStringList types;
    QString resource;
    qint64 left = -1;
    qint64 right = -1;
    qint64 side = -1;

    if (mScope.scope() != Scope::Uid) {
        throw HandlerException("Only UID based fetch is implemented");
    }

    m_streamParser->beginList();

    while (!m_streamParser->atListEnd()) {
        const QByteArray param = m_streamParser->readString();

        if (param == AKONADI_PARAM_RESOURCE) {
            resource = m_streamParser->readUtf8String();
        } else if (param == AKONADI_PARAM_LEFT) {
            left = m_streamParser->readNumber();
        } else if (param == AKONADI_PARAM_RIGHT) {
            right = m_streamParser->readNumber();
        } else if (param == AKONADI_PARAM_SIDE) {
            side = m_streamParser->readNumber();
        } else if (param == AKONADI_PARAM_TYPE) {
            m_streamParser->beginList();
            while (!m_streamParser->atListEnd()) {
                types << m_streamParser->readUtf8String();
            }
        } else {
            return failureResponse(QByteArray("Unknown parameter while creating relation '") + param + QByteArray("'."));
        }
    }

    SelectQueryBuilder<Relation> relationQuery;
    if (side > 0) {
        Query::Condition c;
        c.setSubQueryMode(Query::Or);
        c.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, side);
        c.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, side);
        relationQuery.addCondition(c);
    } else {
        if (left > 0) {
            relationQuery.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, left);
        }
        if (right > 0) {
            relationQuery.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, right);
        }
    }
    if (!types.isEmpty()) {
        relationQuery.addJoin(QueryBuilder::InnerJoin, RelationType::tableName(), Relation::typeIdFullColumnName(), RelationType::idFullColumnName());
        if (types.size() > 1) {
            throw HandlerException("Only a single type is supported");
        }
        relationQuery.addValueCondition(RelationType::nameFullColumnName(), Query::Equals, types.first());
    }
    if (!resource.isEmpty()) {
        Resource res = Resource::retrieveByName(resource);
        if (!res.isValid()) {
            throw HandlerException("Invalid resource");
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
        throw HandlerException("Failed to query for existing relation");
    }
    const Relation::List existingRelations = relationQuery.result();
    Q_FOREACH (const Relation &relation, existingRelations) {
        Response response;

        const QByteArray reply = "RELATIONFETCH (" + relationToByteArray(relation.leftId(), relation.rightId(), relation.relationType().name().toLatin1(), relation.remoteId().toLatin1()) + ')';

        response.setUntagged();
        response.setString(reply);
        responseAvailable(response);
    }

    return successResponse("RELATIONFETCH complete");
}

