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

#include <private/protocol_p.h>

using namespace Akonadi;
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

void CommandContext::setScopeContext(const Protocol::ScopeContext &scopeContext)
{
    if (scopeContext.type() == Protocol::ScopeContext::Collection) {
        mCollection = Collection::retrieveById(scopeContext.id());
    } else if (scopeContext.type() == Protocol::ScopeContext::Tag) {
        mTagId = scopeContext.id();
    }
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
