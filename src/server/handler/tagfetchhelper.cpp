/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagfetchhelper.h"
#include "connection.h"
#include "handler.h"
#include "storage/querybuilder.h"
#include "storage/tagqueryhelper.h"
#include "utils.h"

using namespace Akonadi;
using namespace Akonadi::Server;

TagFetchHelper::TagFetchHelper(Connection *connection, const Scope &scope, const Protocol::TagFetchScope &fetchScope)
    : mConnection(connection)
    , mScope(scope)
    , mFetchScope(fetchScope)
{
}

QueryBuilder TagFetchHelper::buildAttributeQuery() const
{
    QueryBuilder qb(TagAttribute::tableName());
    qb.addColumn(TagAttribute::tagIdFullColumnName());
    qb.addColumn(TagAttribute::typeFullColumnName());
    qb.addColumn(TagAttribute::valueFullColumnName());
    qb.addSortColumn(TagAttribute::tagIdFullColumnName(), Query::Descending);
    qb.addJoin(QueryBuilder::InnerJoin, Tag::tableName(), TagAttribute::tagIdFullColumnName(), Tag::idFullColumnName());
    TagQueryHelper::scopeToQuery(mScope, mConnection->context(), qb);

    if (!qb.exec()) {
        throw HandlerException("Unable to list tag attributes");
    }

    qb.query().next();
    return qb;
}

QueryBuilder TagFetchHelper::buildAttributeQuery(qint64 id, const Protocol::TagFetchScope &fetchScope)
{
    QueryBuilder qb(TagAttribute::tableName());
    qb.addColumn(TagAttribute::tagIdColumn());
    qb.addColumn(TagAttribute::typeColumn());
    qb.addColumn(TagAttribute::valueColumn());
    qb.addSortColumn(TagAttribute::tagIdColumn(), Query::Descending);

    qb.addValueCondition(TagAttribute::tagIdColumn(), Query::Equals, id);
    if (!fetchScope.fetchAllAttributes() && !fetchScope.attributes().isEmpty()) {
        QVariantList typeNames;
        const auto attrs = fetchScope.attributes();
        std::transform(attrs.cbegin(), attrs.cend(), std::back_inserter(typeNames), [](const QByteArray &ba) {
            return QVariant(ba);
        });
        qb.addValueCondition(TagAttribute::typeColumn(), Query::In, typeNames);
    }

    if (!qb.exec()) {
        throw HandlerException("Unable to list tag attributes");
    }

    qb.query().next();
    return qb;
}

QueryBuilder TagFetchHelper::buildTagQuery()
{
    QueryBuilder qb(Tag::tableName());
    qb.addColumn(Tag::idFullColumnName());
    qb.addColumn(Tag::gidFullColumnName());
    qb.addColumn(Tag::parentIdFullColumnName());

    qb.addJoin(QueryBuilder::InnerJoin, TagType::tableName(), Tag::typeIdFullColumnName(), TagType::idFullColumnName());
    qb.addColumn(TagType::nameFullColumnName());

    // Expose tag's remote ID only to resources
    if (mFetchScope.fetchRemoteID() && mConnection->context().resource().isValid()) {
        qb.addColumn(TagRemoteIdResourceRelation::remoteIdFullColumnName());
        Query::Condition joinCondition;
        joinCondition.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(), Query::Equals, mConnection->context().resource().id());
        joinCondition.addColumnCondition(TagRemoteIdResourceRelation::tagIdFullColumnName(), Query::Equals, Tag::idFullColumnName());
        qb.addJoin(QueryBuilder::LeftJoin, TagRemoteIdResourceRelation::tableName(), joinCondition);
    }

    qb.addSortColumn(Tag::idFullColumnName(), Query::Descending);
    TagQueryHelper::scopeToQuery(mScope, mConnection->context(), qb);
    if (!qb.exec()) {
        throw HandlerException("Unable to list tags");
    }

    qb.query().next();
    return qb;
}

QMap<QByteArray, QByteArray> TagFetchHelper::fetchTagAttributes(qint64 tagId, const Protocol::TagFetchScope &fetchScope)
{
    QMap<QByteArray, QByteArray> attributes;

    auto attributeQb = buildAttributeQuery(tagId, fetchScope);
    auto &attributeQuery = attributeQb.query();
    while (attributeQuery.isValid()) {
        attributes.insert(Utils::variantToByteArray(attributeQuery.value(1)), Utils::variantToByteArray(attributeQuery.value(2)));
        attributeQuery.next();
    }
    attributeQuery.finish();
    return attributes;
}

bool TagFetchHelper::fetchTags()
{
    auto tagQb = buildTagQuery();
    auto &tagQuery = tagQb.query();

    std::optional<QueryBuilder> attributeQb;
    if (!mFetchScope.fetchIdOnly()) {
        attributeQb = buildAttributeQuery();
    }

    while (tagQuery.isValid()) {
        const qint64 tagId = tagQuery.value(0).toLongLong();
        Protocol::FetchTagsResponse response;
        response.setId(tagId);
        if (!mFetchScope.fetchIdOnly()) {
            response.setGid(Utils::variantToByteArray(tagQuery.value(1)));
            if (tagQuery.value(2).isNull()) {
                // client indicates invalid or null parent as ID -1
                response.setParentId(-1);
            } else {
                response.setParentId(tagQuery.value(2).toLongLong());
            }
            response.setType(Utils::variantToByteArray(tagQuery.value(3)));
            if (mFetchScope.fetchRemoteID() && mConnection->context().resource().isValid()) {
                response.setRemoteId(Utils::variantToByteArray(tagQuery.value(4)));
            }

            if (attributeQb.has_value()) {
                QMap<QByteArray, QByteArray> tagAttributes;
                auto &attributeQuery = attributeQb->query();
                while (attributeQuery.isValid()) {
                    const qint64 id = attributeQuery.value(0).toLongLong();
                    if (id > tagId) {
                        attributeQuery.next();
                        continue;
                    } else if (id < tagId) {
                        break;
                    }

                    tagAttributes.insert(Utils::variantToByteArray(attributeQuery.value(1)), Utils::variantToByteArray(attributeQuery.value(2)));
                    attributeQuery.next();
                }

                response.setAttributes(tagAttributes);
            }
        }

        mConnection->sendResponse(std::move(response));

        tagQuery.next();
    }

    return true;
}
