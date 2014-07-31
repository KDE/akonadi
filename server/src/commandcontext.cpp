/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "commandcontext.h"
#include "imapstreamparser.h"
#include "handler.h"
#include "storage/selectquerybuilder.h"

#include "libs/protocol_p.h"

using namespace Akonadi::Server;

CommandContext::CommandContext()
    : mTagId(-1)
{
}

CommandContext::~CommandContext()
{
}

void CommandContext::setResource(const Resource &resource)
{
    mResource = resource;
}

Resource CommandContext::resource() const
{
    return mResource;
}

void CommandContext::setCollection(const Collection &collection)
{
    mCollection = collection;
}

qint64 CommandContext::collectionId() const
{
    return mCollection.id();
}

Collection CommandContext::collection() const
{
    return mCollection;
}

void CommandContext::setTag(qint64 tagId)
{
    mTagId = tagId;
}

qint64 CommandContext::tagId() const
{
    return mTagId;
}

Tag CommandContext::tag() const
{
    if (mTagId == -1) {
        return Tag();
    }

    return Tag::retrieveById(mTagId);
}

bool CommandContext::isEmpty() const
{
    return !mCollection.isValid() && mTagId < 0;
}

void CommandContext::parseContext(ImapStreamParser *parser)
{
    // Context
    if (!parser->hasString()) {
        return;
    }

    const QByteArray param = parser->peekString();

    if (param == AKONADI_PARAM_COLLECTIONID) {
        parser->readString(); // Read the param
        bool ok = false;
        const qint64 colId = parser->readNumber(&ok);
        if (!ok) {
            throw HandlerException("Invalid FETCH collection ID");
        }
        const Collection col = Collection::retrieveById(colId);
        if (!col.isValid()) {
            throw HandlerException("No such collection");
        }
        setCollection(col);
    } else if (param == AKONADI_PARAM_COLLECTION) {
        if (!resource().isValid()) {
            throw HandlerException("Only resources can use REMOTEID");
        }
        parser->readString(); // Read the param
        const QByteArray rid = parser->readString();
        SelectQueryBuilder<Collection> qb;
        qb.addValueCondition(Collection::remoteIdColumn(), Query::Equals, QString::fromUtf8(rid));
        qb.addValueCondition(Collection::resourceIdColumn(), Query::Equals, resource().id());
        if (!qb.exec()) {
            throw HandlerException("Failed to select collection");
        }
        Collection::List results = qb.result();
        if (results.count() != 1) {
            throw HandlerException(QByteArray::number(results.count()) + " collections found");
        }
        setCollection(results.first());
    }

    if (param == AKONADI_PARAM_TAGID) {
        parser->readString(); // Read the param
        bool ok = false;
        const qint64 tagId = parser->readNumber(&ok);
        if (!ok) {
            throw HandlerException("Invalid FETCH tag");
        }
        if (!Tag::exists(tagId)) {
            throw HandlerException("No such tag");
        }
        setTag(tagId);
    }
}
