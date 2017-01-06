/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ABSTRACTSEARCHENGINE_H
#define AKONADI_ABSTRACTSEARCHENGINE_H

#include <qglobal.h>

namespace Akonadi
{
namespace Server
{

class Collection;

/**
 * Abstract interface for search engines.
 * Executed in the main thread. Must not block.
 */
class AbstractSearchEngine
{
public:
    virtual ~AbstractSearchEngine()
    {
    }

    /**
     * Adds the given @p collection to the search.
     *
     * @returns true if the collection was added successfully, false otherwise.
     */
    virtual void addSearch(const Collection &collection) = 0;

    /**
     * Removes the collection with the given @p id from the search.
     *
     * @returns true if the collection was removed successfully, false otherwise.
     */
    virtual void removeSearch(qint64 id) = 0;
};

} // namespace Server
} // namespace Akonadi

#endif
