/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagcreatehandler.h"

#include "connection.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"
#include "storage/transaction.h"
#include "tagfetchhelper.h"

#include "private/imapset_p.h"
#include "private/scope_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

TagCreateHandler::TagCreateHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool TagCreateHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::CreateTagCommand>(m_command);

    if (!cmd.remoteId().isEmpty() && !connection()->context().resource().isValid()) {
        return failureResponse(QStringLiteral("Only resources can create tags with remote ID"));
    }

    Transaction trx(storageBackend(), QStringLiteral("TAGAPPEND"));

    TagType tagType;
    if (!cmd.type().isEmpty()) {
        const QString typeName = QString::fromUtf8(cmd.type());
        tagType = TagType::retrieveByNameOrCreate(typeName);
        if (!tagType.isValid()) {
            return failureResponse(QStringLiteral("Unable to create tagtype '") % typeName % QStringLiteral("'"));
        }
    }

    qint64 tagId = -1;
    const QString gid = QString::fromUtf8(cmd.gid());
    if (cmd.merge()) {
        QueryBuilder qb(Tag::tableName());
        qb.addColumn(Tag::idColumn());
        qb.addValueCondition(Tag::gidColumn(), Query::Equals, gid);
        if (!qb.exec()) {
            return failureResponse("Unable to list tags");
        }
        if (qb.query().next()) {
            tagId = qb.query().value(0).toLongLong();
        }
        qb.query().finish();
    }
    if (tagId < 0) {
        Tag insertedTag;
        insertedTag.setGid(gid);
        if (cmd.parentId() >= 0) {
            insertedTag.setParentId(cmd.parentId());
        }
        if (tagType.isValid()) {
            insertedTag.setTypeId(tagType.id());
        }
        if (!insertedTag.insert(&tagId)) {
            return failureResponse("Failed to store tag");
        }

        const Protocol::Attributes attrs = cmd.attributes();
        for (auto iter = attrs.cbegin(), end = attrs.cend(); iter != end; ++iter) {
            TagAttribute attribute;
            attribute.setTagId(tagId);
            attribute.setType(iter.key());
            attribute.setValue(iter.value());
            if (!attribute.insert()) {
                return failureResponse("Failed to store tag attribute");
            }
        }

        storageBackend()->notificationCollector()->tagAdded(insertedTag);
    }

    if (!cmd.remoteId().isEmpty()) {
        const qint64 resourceId = connection()->context().resource().id();

        CountQueryBuilder qb(TagRemoteIdResourceRelation::tableName());
        qb.addValueCondition(TagRemoteIdResourceRelation::tagIdColumn(), Query::Equals, tagId);
        qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdColumn(), Query::Equals, resourceId);
        if (!qb.exec()) {
            return failureResponse("Failed to query for existing TagRemoteIdResourceRelation entries");
        }
        const bool exists = (qb.result() > 0);

        // If the relation is already existing simply update it (can happen if a resource simply creates the tag again while enabling merge)
        bool ret = false;
        if (exists) {
            // Simply using update() doesn't work since TagRemoteIdResourceRelation only takes the tagId for identification of the column
            QueryBuilder qb(TagRemoteIdResourceRelation::tableName(), QueryBuilder::Update);
            qb.addValueCondition(TagRemoteIdResourceRelation::tagIdColumn(), Query::Equals, tagId);
            qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdColumn(), Query::Equals, resourceId);
            qb.setColumnValue(TagRemoteIdResourceRelation::remoteIdColumn(), QString::fromUtf8(cmd.remoteId()));
            ret = qb.exec();
        } else {
            TagRemoteIdResourceRelation rel;
            rel.setTagId(tagId);
            rel.setResourceId(resourceId);
            rel.setRemoteId(QString::fromUtf8(cmd.remoteId()));
            ret = rel.insert();
        }
        if (!ret) {
            return failureResponse("Failed to store tag remote ID");
        }
    }

    trx.commit();

    Scope scope;
    ImapSet set;
    set.add(QList<qint64>() << tagId);
    scope.setUidSet(set);

    Protocol::TagFetchScope fetchScope;
    fetchScope.setFetchRemoteID(true);
    fetchScope.setFetchAllAttributes(true);

    TagFetchHelper helper(connection(), scope, fetchScope);
    if (!helper.fetchTags()) {
        return failureResponse("Failed to fetch the new tag");
    }

    return successResponse<Protocol::CreateTagResponse>();
}
