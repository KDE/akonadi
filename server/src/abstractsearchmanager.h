/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ABSTRACTSEARCHMANAGER_H
#define AKONADI_ABSTRACTSEARCHMANAGER_H

#include <QtCore/QGlobalStatic>

namespace Akonadi {

class Location;

/**
 * AbstractSearchManager is an abstract interface for search managers.
 *
 * Currently we have the XesamManager and NepomukManager as implementations,
 * but both don't work completely.
 */
class AbstractSearchManager
{
  public:
    /**
     * Destroys the abstract search manager.
     */
    virtual ~AbstractSearchManager();

    /**
     * Returns a global instance of the search manager implementation.
     */
    static AbstractSearchManager* instance();

    /**
     * Adds the given @p location to the search.
     *
     * @returns true if the location was added successfully, false otherwise.
     */
    virtual bool addSearch( const Location &location ) = 0;

    /**
     * Removes the location with the given @p id from the search.
     *
     * @returns true if the location was removed successfully, false otherwise.
     */
    virtual bool removeSearch( qint64 id ) = 0;

  protected:
    static AbstractSearchManager* mInstance;
};

/**
 * DummySearchManager is a dummy implementation of AbstractSearchManager
 * which does nothing.
 */
class DummySearchManager : public AbstractSearchManager
{
  public:
    DummySearchManager();
    bool addSearch( const Location &location );
    bool removeSearch( qint64 id );
};

}

#endif
