/*
    SPDX-FileCopyrightText: 2013 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QString>

namespace Akonadi
{
class Item;

/**
 * @internal
 * Extracts the GID of an object contained in an akonadi item using a plugin that implements the GidExtractorInterface.
 */
class GidExtractor
{
public:
    /**
     * Extracts the GID from @p item. using an extractor plugin.
     */
    static QString extractGid(const Item &item);

    /**
     * Extracts the gid from @p item.
     *
     * If the item has a GID set, that GID will be returned.
     * If the item has no GID set, and the item has a payload, the GID is extracted using extractGid().
     * If the item has no GID set and no payload, a default constructed QString is returned.
     */
    static QString getGid(const Item &item);
};

}
