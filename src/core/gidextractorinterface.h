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

#ifndef GIDEXTRACTORINTERFACE_H
#define GIDEXTRACTORINTERFACE_H

#include <QtCore/QObject>

namespace Akonadi {

class Item;

/**
 * @short An interface to extract the GID of an object contained in an akonadi item.
 *
 * @author Christian Mollekopf <mollekopf@kolabsys.com>
 * @since 4.11
 */
class GidExtractorInterface
{
public:
    /**
     * Destructor.
     */
    virtual ~GidExtractorInterface()
    {
    }
    /**
     * Extracts the globally unique id of @p item
     *
     * If you want to clear the gid from the database return QString("").
     */
    virtual QString extractGid(const Item &item) const = 0;
};

}

Q_DECLARE_INTERFACE(Akonadi::GidExtractorInterface, "org.freedesktop.Akonadi.GidExtractorInterface/1.0")

#endif
