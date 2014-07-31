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
#include "libs/protocol_p.h"

using namespace Akonadi::Server;

TagStore::TagStore()
    : Handler()
{
}

TagStore::~TagStore()
{
}

bool TagStore::parseStream()
{
    const qint64 tagId = m_streamParser->readNumber();

    if (!m_streamParser->hasList()) {
        failureResponse("No changes to store");
        return false;
    }

    Tag changedTag = Tag::retrieveById(tagId);
    if (!changedTag.isValid()) {
        throw HandlerException("No such tag");
    }

    // Retrieve all tag's attributes
    const TagAttribute::List attributes = TagAttribute::retrieveFiltered(TagAttribute::tagIdFullColumnName(), tagId);
    QMap<QByteArray, TagAttribute> attributesMap;
    Q_FOREACH (const TagAttribute &attribute, attributes) {
        attributesMap.insert(attribute.type(), attribute);
    }

    m_streamParser->beginList();
    while (!m_streamParser->atListEnd()) {
        const QByteArray attr = m_streamParser->readString();

        if (attr == AKONADI_PARAM_PARENT) {
            const qint64 parent = m_streamParser->readNumber();
            changedTag.setParentId(parent);
        } else if (attr == AKONADI_PARAM_GID) {
            throw HandlerException("Changing tag GID is not allowed");
        } else if (attr == AKONADI_PARAM_UID) {
            throw HandlerException("Changing tag UID is not allowed");
        } else {
            if (attr.startsWith('-')) {
                const QByteArray attrName = attr.mid(1);
                if (attributesMap.contains(attrName)) {
                    TagAttribute attribute = attributesMap.value(attrName);
                    TagAttribute::remove(attribute.id());
                }
            } else if (attributesMap.contains(attr)) {
                TagAttribute attribute = attributesMap.value(attr);
                attribute.setValue(m_streamParser->readString());
                attribute.update();
            } else {
                TagAttribute attribute;
                attribute.setTagId(tagId);
                attribute.setType(attr);
                attribute.setValue(m_streamParser->readString());
                attribute.insert();
            }
        }
    }

    DataStore::self()->notificationCollector()->tagChanged(changedTag);

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
    response.setString("TAGSTORE completed");
    Q_EMIT responseAvailable(response);
    return true;
}
