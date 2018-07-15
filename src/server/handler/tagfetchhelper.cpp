/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "tagfetchhelper.h"
#include "handler.h"
#include "connection.h"
#include "utils.h"
#include "storage/querybuilder.h"
#include "storage/tagqueryhelper.h"

using namespace Akonadi;
using namespace Akonadi::Server;

TagFetchHelper::TagFetchHelper(Connection *connection, const Scope &scope)
    : mConnection(connection)
    , mScope(scope)
{
}

QSqlQuery TagFetchHelper::buildAttributeQuery() const
{
    QueryBuilder qb(TagAttribute::tableName());
    qb.addColumn(TagAttribute::tagIdFullColumnName());
    qb.addColumn(TagAttribute::typeFullColumnName());
    qb.addColumn(TagAttribute::valueFullColumnName());
    qb.addSortColumn(TagAttribute::tagIdFullColumnName(), Query::Descending);
    qb.addJoin(QueryBuilder::InnerJoin, Tag::tableName(),
               TagAttribute::tagIdFullColumnName(), Tag::idFullColumnName());
    TagQueryHelper::scopeToQuery(mScope, mConnection->context(), qb);

    if (!qb.exec()) {
        throw HandlerException("Unable to list tag attributes");
    }

    qb.query().next();
    return qb.query();
}

QSqlQuery TagFetchHelper::buildAttributeQuery(qint64 id)
{
    QueryBuilder qb(TagAttribute::tableName());
    qb.addColumn(TagAttribute::tagIdColumn());
    qb.addColumn(TagAttribute::typeColumn());
    qb.addColumn(TagAttribute::valueColumn());
    qb.addSortColumn(TagAttribute::tagIdColumn(), Query::Descending);

    qb.addValueCondition(TagAttribute::tagIdColumn(), Query::Equals, id);

    if (!qb.exec()) {
        throw HandlerException("Unable to list tag attributes");
    }

    qb.query().next();
    return qb.query();
}

QSqlQuery TagFetchHelper::buildTagQuery()
{
    QueryBuilder qb(Tag::tableName());
    qb.addColumn(Tag::idFullColumnName());
    qb.addColumn(Tag::gidFullColumnName());
    qb.addColumn(Tag::parentIdFullColumnName());

    qb.addJoin(QueryBuilder::InnerJoin, TagType::tableName(),
               Tag::typeIdFullColumnName(), TagType::idFullColumnName());
    qb.addColumn(TagType::nameFullColumnName());

    // Expose tag's remote ID only to resources
    if (mConnection->context()->resource().isValid()) {
        qb.addColumn(TagRemoteIdResourceRelation::remoteIdFullColumnName());
        Query::Condition joinCondition;
        joinCondition.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(),
                                        Query::Equals, mConnection->context()->resource().id());
        joinCondition.addColumnCondition(TagRemoteIdResourceRelation::tagIdFullColumnName(),
                                         Query::Equals, Tag::idFullColumnName());
        qb.addJoin(QueryBuilder::LeftJoin, TagRemoteIdResourceRelation::tableName(), joinCondition);
    }

    qb.addSortColumn(Tag::idFullColumnName(), Query::Descending);
    TagQueryHelper::scopeToQuery(mScope, mConnection->context(), qb);
    if (!qb.exec()) {
        throw HandlerException("Unable to list tags");
    }

    qb.query().next();
    return qb.query();
}

QMap<QByteArray, QByteArray> TagFetchHelper::fetchTagAttributes(qint64 tagId)
{
    QMap<QByteArray, QByteArray> attributes;

    QSqlQuery attributeQuery = buildAttributeQuery(tagId);
    while (attributeQuery.isValid()) {
        attributes.insert(Utils::variantToByteArray(attributeQuery.value(1)),
                          Utils::variantToByteArray(attributeQuery.value(2)));
        attributeQuery.next();
    }
    return attributes;
}

bool TagFetchHelper::fetchTags()
{

    QSqlQuery tagQuery = buildTagQuery();
    QSqlQuery attributeQuery = buildAttributeQuery();

    while (tagQuery.isValid()) {
        const qint64 tagId = tagQuery.value(0).toLongLong();
        Protocol::FetchTagsResponse response;
        response.setId(tagId);
        response.setGid(Utils::variantToByteArray(tagQuery.value(1)));
        response.setParentId(tagQuery.value(2).toLongLong());
        response.setType(Utils::variantToByteArray(tagQuery.value(3)));
        if (mConnection->context()->resource().isValid()) {
            response.setRemoteId(Utils::variantToByteArray(tagQuery.value(4)));
        }

        QMap<QByteArray, QByteArray> tagAttributes;
        while (attributeQuery.isValid()) {
            const qint64 id = attributeQuery.value(0).toLongLong();
            if (id > tagId) {
                attributeQuery.next();
                continue;
            } else if (id < tagId) {
                break;
            }

            tagAttributes.insert(Utils::variantToByteArray(attributeQuery.value(1)),
                                 Utils::variantToByteArray(attributeQuery.value(2)));
            attributeQuery.next();
        }

        response.setAttributes(tagAttributes);

        mConnection->sendResponse(std::move(response));

        tagQuery.next();
    }

    return true;
}
