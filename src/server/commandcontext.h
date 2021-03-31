/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

#include "entities.h"

#include <optional>

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
    void setResource(const Resource &resource);
    Resource resource() const;

    bool setScopeContext(const Protocol::ScopeContext &scopeContext);

    void setCollection(const Collection &collection);
    qint64 collectionId() const;
    Collection collection() const;

    void setTag(std::optional<qint64> tagId);
    std::optional<qint64> tagId() const;
    Tag tag() const;

    bool isEmpty() const;

private:
    Resource mResource;
    Collection mCollection;
    std::optional<qint64> mTagId;
};

}

}

