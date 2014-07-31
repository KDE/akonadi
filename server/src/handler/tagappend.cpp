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

#include "tagappend.h"
#include "tagfetchhelper.h"
#include "connection.h"
#include "imapstreamparser.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"
#include "storage/countquerybuilder.h"
#include "entities.h"
#include "libs/protocol_p.h"

using namespace Akonadi::Server;

TagAppend::TagAppend()
    : Handler()
{
}

TagAppend::~TagAppend()
{
}

bool TagAppend::parseStream()
{
    m_streamParser->beginList();

    typedef QPair<QByteArray, QByteArray> AttributePair;
    QList<AttributePair> attributes;
    QString remoteId;
    bool merge = false;
    QString gid;
    QByteArray type;
    qint64 parentId = -1;

    while (!m_streamParser->atListEnd()) {
        const QByteArray param = m_streamParser->readString();

        if (param == AKONADI_PARAM_GID) {
            gid = QString::fromLatin1(m_streamParser->readString());
        } else if (param == AKONADI_PARAM_PARENT) {
            parentId = m_streamParser->readNumber();
        } else if (param == AKONADI_PARAM_REMOTEID) {
            if (!connection()->context()->resource().isValid()) {
                throw HandlerException("Only resource can create tag with remote ID");
            }
            remoteId = QString::fromLatin1(m_streamParser->readString());
        } else if (param == AKONADI_PARAM_MERGE) {
            merge = true;
        } else if (param == AKONADI_PARAM_MIMETYPE) {
            type = m_streamParser->readString();
        } else {
            attributes << qMakePair(param, m_streamParser->readString());
        }
    }

    TagType tagType;
    if (!type.isEmpty()) {
        tagType = TagType::retrieveByName(QString::fromLatin1(type));
        if (!tagType.isValid()) {
            TagType t(QString::fromLatin1(type));
            if (!t.insert()) {
                return failureResponse(QByteArray("Unable to create tagtype '") + type + QByteArray("'."));
            }
            tagType = t;
        }
    }

    qint64 tagId = -1;
    if (merge) {
        QueryBuilder qb(Tag::tableName());
        qb.addColumn(Tag::idColumn());
        qb.addValueCondition(Tag::gidColumn(), Query::Equals, gid);
        if (!qb.exec()) {
            throw HandlerException("Unable to list tags");
        }
        if (qb.query().next()) {
            tagId = qb.query().value(0).toLongLong();
        }
    }
    if (tagId < 0) {
        Tag insertedTag;
        insertedTag.setGid(gid);
        if (parentId >= 0) {
            insertedTag.setParentId(parentId);
        }
        if (tagType.isValid()) {
            insertedTag.setTypeId(tagType.id());
        }
        if (!insertedTag.insert(&tagId)) {
            throw HandlerException("Failed to store tag");
        }

        Q_FOREACH (const AttributePair &pair, attributes) {
            TagAttribute attribute;
            attribute.setTagId(tagId);
            attribute.setType(pair.first);
            attribute.setValue(pair.second);
            if (!attribute.insert()) {
                throw HandlerException("Failed to store tag attribute");
            }
        }

        DataStore::self()->notificationCollector()->tagAdded(insertedTag);
    }

    if (!remoteId.isEmpty()) {
        const qint64 resourceId = connection()->context()->resource().id();

        CountQueryBuilder qb(TagRemoteIdResourceRelation::tableName());
        qb.addValueCondition(TagRemoteIdResourceRelation::tagIdColumn(), Query::Equals, tagId);
        qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdColumn(), Query::Equals, resourceId);
        if (!qb.exec()) {
            throw HandlerException("Failed to query for existing TagRemoteIdResourceRelation entries");
        }
        const bool exists = (qb.result() > 0);

        //If the relation is already existing simply update it (can happen if a resource simply creates the tag again while enabling merge)
        bool ret = false;
        if (exists) {
            //Simply using update() doesn't work since TagRemoteIdResourceRelation only takes the tagId for identification of the column
            QueryBuilder qb(TagRemoteIdResourceRelation::tableName(), QueryBuilder::Update);
            qb.addValueCondition(TagRemoteIdResourceRelation::tagIdColumn(), Query::Equals, tagId);
            qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdColumn(), Query::Equals, resourceId);
            qb.setColumnValue(TagRemoteIdResourceRelation::remoteIdColumn(), remoteId);
            ret = qb.exec();
        } else {
            TagRemoteIdResourceRelation rel;
            rel.setTagId(tagId);
            rel.setResourceId(resourceId);
            rel.setRemoteId(remoteId);
            ret = rel.insert();
        }
        if (!ret) {
            throw HandlerException("Failed to store tag remote ID");
        }
    }

    ImapSet set;
    set.add(QVector<qint64>() << tagId);
    TagFetchHelper helper(connection(), set);
    connect(&helper, SIGNAL(responseAvailable(Akonadi::Server::Response)),
            this, SIGNAL(responseAvailable(Akonadi::Server::Response)));

    if (!helper.fetchTags(AKONADI_CMD_TAGFETCH)) {
        return false;
    }

    Response response;
    response.setTag(tag());
    response.setSuccess();
    response.setString("Append completed");
    Q_EMIT responseAvailable(response);
    return true;
}
