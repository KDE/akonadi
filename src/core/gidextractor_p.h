/*
    Author: (2013) Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef GIDEXTRACTOR_H
#define GIDEXTRACTOR_H

#include <QtCore/QString>

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

#endif
