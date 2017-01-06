/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef COMMANDCONTEXT_H
#define COMMANDCONTEXT_H

#include "entities.h"

namespace Akonadi
{

namespace Protocol
{
class ScopeContext;
}

namespace Server
{

class CommandContext
{
public:
    CommandContext();

    void setResource(const Resource &resource);
    Resource resource() const;

    bool setScopeContext(const Protocol::ScopeContext &scopeContext);

    void setCollection(const Collection &collection);
    qint64 collectionId() const;
    Collection collection() const;

    void setTag(qint64 tagId);
    qint64 tagId() const;
    Tag tag() const;

    bool isEmpty() const;

private:
    Resource mResource;
    Collection mCollection;
    qint64 mTagId;
};

}

}

#endif // COMMANDCONTEXT_H
