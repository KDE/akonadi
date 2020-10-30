/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "commandcontext.h"
#include "storage/selectquerybuilder.h"

#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

void CommandContext::setResource(const Resource &resource)
{
    mResource = resource;
}

Resource CommandContext::resource() const
{
    return mResource;
}

bool CommandContext::setScopeContext(const Protocol::ScopeContext &scopeContext)
{
    if (scopeContext.hasContextId(Protocol::ScopeContext::Collection)) {
        mCollection = Collection::retrieveById(scopeContext.contextId(Protocol::ScopeContext::Collection));
    } else if (scopeContext.hasContextRID(Protocol::ScopeContext::Collection)) {
        if (mResource.isValid()) {
            SelectQueryBuilder<Collection> qb;
            qb.addValueCondition(Collection::remoteIdColumn(), Query::Equals, scopeContext.contextRID(Protocol::ScopeContext::Collection));
            qb.addValueCondition(Collection::resourceIdColumn(), Query::Equals, mResource.id());
            qb.exec();
            Collection::List cols = qb.result();
            if (cols.isEmpty()) {
                // error
                return false;
            }
            mCollection = cols.at(0);
        } else {
            return false;
        }
    }

    if (scopeContext.hasContextId(Protocol::ScopeContext::Tag)) {
        mTagId = scopeContext.contextId(Protocol::ScopeContext::Tag);
    }

    return true;
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

void CommandContext::setTag(std::optional<qint64> tagId)
{
    mTagId = tagId;
}

std::optional<qint64> CommandContext::tagId() const
{
    return mTagId;
}

Tag CommandContext::tag() const
{
    return mTagId.has_value() ? Tag::retrieveById(*mTagId) : Tag();
}

bool CommandContext::isEmpty() const
{
    return !mCollection.isValid() && !mTagId.has_value();
}

