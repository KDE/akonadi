/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagmodifyhandler.h"

#include "tagfetchhelper.h"
#include "connection.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"
#include <shared/akranges.h>

#include <private/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

TagModifyHandler::TagModifyHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

bool TagModifyHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::ModifyTagCommand>(m_command);

    Tag changedTag = Tag::retrieveById(cmd.tagId());
    if (!changedTag.isValid()) {
        return failureResponse("No such tag");
    }

    QSet<QByteArray> changes;

    // Retrieve all tag's attributes
    const TagAttribute::List attributes = TagAttribute::retrieveFiltered(TagAttribute::tagIdFullColumnName(), cmd.tagId());
    const auto attributesMap = attributes | Views::transform([](const auto &attr) { return std::make_pair(attr.type(), attr); }) | Actions::toQMap;

    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::ParentId) {
        if (cmd.parentId() != changedTag.parentId()) {
            changedTag.setParentId(cmd.parentId());
            changes << AKONADI_PARAM_PARENT;
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::Type) {
        TagType type = TagType::retrieveById(changedTag.typeId());
        const QString newTypeName = QString::fromUtf8(cmd.type());
        if (newTypeName != type.name()) {
            const TagType newType = TagType::retrieveByNameOrCreate(newTypeName);
            if (!newType.isValid()) {
                return failureResponse("Failed to create new tag type");
            }
            changedTag.setTagType(newType);
            changes << AKONADI_PARAM_MIMETYPE;
        }
    }

    bool tagRemoved = false;
    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::RemoteId) {
        if (!connection()->context().resource().isValid()) {
            return failureResponse("Only resources can change tag remote ID");
        }

        //Simply using remove() doesn't work since we need two arguments
        QueryBuilder qb(TagRemoteIdResourceRelation::tableName(), QueryBuilder::Delete);
        qb.addValueCondition(TagRemoteIdResourceRelation::tagIdColumn(), Query::Equals, cmd.tagId());
        qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdColumn(), Query::Equals, connection()->context().resource().id());
        qb.exec();

        if (!cmd.remoteId().isEmpty()) {
            TagRemoteIdResourceRelation remoteIdRelation;
            remoteIdRelation.setRemoteId(QString::fromUtf8(cmd.remoteId()));
            remoteIdRelation.setResourceId(connection()->context().resource().id());
            remoteIdRelation.setTag(changedTag);
            if (!remoteIdRelation.insert()) {
                return failureResponse("Failed to insert remotedid resource relation");
            }
        } else {
            const int tagRidsCount = TagRemoteIdResourceRelation::count(TagRemoteIdResourceRelation::tagIdColumn(), changedTag.id());
            // We just removed the last RID of the tag, which means that no other
            // resource owns this tag, so we have to remove it to simulate tag
            // removal
            if (tagRidsCount == 0) {
                if (!storageBackend()->removeTags(Tag::List() << changedTag)) {
                    return failureResponse("Failed to remove tag");
                }
                tagRemoved = true;
            }
        }
        // Do not notify about remoteid changes, otherwise we bounce back and forth
        // between resources recording it's change and updating the remote id.
    }

    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::RemovedAttributes) {
        Q_FOREACH (const QByteArray &attrName, cmd.removedAttributes()) {
            TagAttribute attribute = attributesMap.value(attrName);
            TagAttribute::remove(attribute.id());
            changes << attrName;
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::Attributes) {
        const QMap<QByteArray, QByteArray> attrs = cmd.attributes();
        for (auto iter = attrs.cbegin(), end = attrs.cend(); iter != end; ++iter) {
            if (attributesMap.contains(iter.key())) {
                TagAttribute attribute = attributesMap.value(iter.key());
                attribute.setValue(iter.value());
                if (!attribute.update()) {
                    return failureResponse("Failed to update attribute");
                }
            } else {
                TagAttribute attribute;
                attribute.setTagId(cmd.tagId());
                attribute.setType(iter.key());
                attribute.setValue(iter.value());
                if (!attribute.insert()) {
                    return failureResponse("Failed to insert attribute");
                }
            }
            changes << iter.key();
        }
    }

    if (!tagRemoved) {
        if (!changedTag.update()) {
            return failureResponse("Failed to store changes");
        }
        if (!changes.isEmpty()) {
            storageBackend()->notificationCollector()->tagChanged(changedTag);
        }

        ImapSet set;
        set.add(QVector<qint64>() << cmd.tagId());

        Protocol::TagFetchScope fetchScope;
        fetchScope.setFetchRemoteID(true);
        fetchScope.setFetchAllAttributes(true);

        Scope scope;
        scope.setUidSet(set);
        TagFetchHelper helper(connection(), scope, fetchScope);
        if (!helper.fetchTags()) {
            return failureResponse("Failed to fetch response");
        }
    } else {
        successResponse<Protocol::DeleteTagResponse>();
    }

    return successResponse<Protocol::ModifyTagResponse>();
}
