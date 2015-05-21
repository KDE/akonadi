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

#include "tagstore.h"
#include "scope.h"
#include "tagfetchhelper.h"
#include "imapstreamparser.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"

#include <private/protocol_p.h>
#include "connection.h"

using namespace Akonadi;
using namespace Akonadi::Server;

bool TagStore::parseStream()
{
    Protocol::ModifyTagCommand cmd;
    mInStream >> cmd;

    Tag changedTag = Tag::retrieveById(cmd.tagId());
    if (!changedTag.isValid()) {
        return failureResponse<Protocol::ModifyTagResponse>(QStringLiteral("No such tag"));
    }

    QSet<QByteArray> changes;

    // Retrieve all tag's attributes
    const TagAttribute::List attributes = TagAttribute::retrieveFiltered(TagAttribute::tagIdFullColumnName(), cmd.tagId());
    QMap<QByteArray, TagAttribute> attributesMap;
    Q_FOREACH (const TagAttribute &attribute, attributes) {
        attributesMap.insert(attribute.type(), attribute);
    }

    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::ParentId) {
        if (cmd.parentId() != changedTag.parentId()) {
            changedTag.setParentId(cmd.parentId());
            changes << AKONADI_PARAM_PARENT;
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::Type) {
        TagType type = TagType::retrieveById(changedTag.typeId());
        if (cmd.type() != type.name()) {
            TagType newType = TagType::retrieveByName(cmd.type());
            if (!newType.isValid()) {
                newType.setName(cmd.type());
                if (!newType.insert()) {
                    return failureResponse<Protocol::ModifyTagResponse>(
                        QStringLiteral("Failed to create new tag type"));
                }
            }
            changedTag.setTagType(newType);
            changes << AKONADI_PARAM_MIMETYPE;
        }
    }

    bool tagRemoved = false;
    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::RemoteId) {
        if (!connection()->context()->resource().isValid()) {
            return failureResponse<Protocol::ModifyTagResponse>(
                QStringLiteral("Only resources can change tag remote ID"));
        }

        //Simply using remove() doesn't work since we need two arguments
        QueryBuilder qb(TagRemoteIdResourceRelation::tableName(), QueryBuilder::Delete);
        qb.addValueCondition(TagRemoteIdResourceRelation::tagIdColumn(), Query::Equals, cmd.tagId());
        qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdColumn(), Query::Equals,connection()->context()->resource().id());
        qb.exec();

        if (!cmd.remoteId().isEmpty()) {
            TagRemoteIdResourceRelation remoteIdRelation;
            remoteIdRelation.setRemoteId(cmd.remoteId());
            remoteIdRelation.setResourceId(connection()->context()->resource().id());
            remoteIdRelation.setTag(changedTag);
            if (!remoteIdRelation.insert()) {
                throw HandlerException( "Failed to insert remotedid resource relation" );
            }
        } else {
            const int tagRidsCount = TagRemoteIdResourceRelation::count(TagRemoteIdResourceRelation::tagIdColumn(), changedTag.id());
            // We just removed the last RID of the tag, which means that no other
            // resource owns this tag, so we have to remove it to simulate tag
            // removal
            if (tagRidsCount == 0) {
                if (!DataStore::self()->removeTags(Tag::List() << changedTag)) {
                    throw HandlerException( "Failed to remove tag" );
                }
                tagRemoved = true;
            }
        }
        //Do not notify about remoteid changes, otherwise we bounce back and forth
        // between resources recording it's change and updating the remote id.
    }

    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::RemovedAttributes) {
        for (const QByteArray &attrName : cmd.removedAttributes()) {
            TagAttribute attribute = attributesMap.value(attrName);
            TagAttribute::remove(attribute.id());
            changes << attrName;
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyTagCommand::Attributes) {
        for (const QPair<QByteArray, QByteArray> &attr : cmd.attributes()) {
            if (attributesMap.contains(attr.first)) {
                TagAttribute attribute = attributesMap.value(attr);
                attribute.setValue(attr.second);
                if (!attribute.update()) {
                    return failureResponse<Protocol::ModifyTagResponse>(
                        QStringLiteral(("Failed to update attribute"));
                }
            } else {
                TagAttribute attribute;
                attribute.setTagId(cmd.tagId());
                attribute.setType(attr.first);
                attribute.setValue(attr.second);
                if (!attribute.insert()) {
                    return failureResponse<Protocol::ModifyTagResponse>(
                        QStringLiteral("Failed to insert attribute"));
                }
            }
            changes << attr.first;
        }
    }

    if (!tagRemoved) {
        if (!changedTag.update()) {
            return failureResponse<Protocol::ModifyTagResponse>(
                QStringLiteral("Failed to store changes")();
        }
        if (!changes.isEmpty()) {
            DataStore::self()->notificationCollector()->tagChanged(changedTag);
        }

        Scope scope(Scope::Uid);
        scope.setUidSet(QVector<qint64>() << cmd.tagId());
        TagFetchHelper helper(connection(), scope);
        // FIXME BIN: No notify
        connect(&helper, SIGNAL(responseAvailable(Akonadi::Server::Response)),
                this, SIGNAL(responseAvailable(Akonadi::Server::Response)));
        if (!helper.fetchTags(AKONADI_CMD_TAGFETCH)) {
            return false;
        }
    } else {
        mOutStream << Protocol::DeleteTagResponse();
    }

    mOutStream << Protocol::ModifyTagResponse();
    return true;
}
