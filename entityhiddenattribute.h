/******************************************************************************
 *
 *  Copyright (c) 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA, 02110-1301, USA.
 *
 *****************************************************************************/

#ifndef AKONADI_ENTITYHIDDENATTRIBUTE_H
#define AKONADI_ENTITYHIDDENATTRIBUTE_H

#include <akonadi/attribute.h>

namespace Akonadi {

/**
 * @short Attribute signalling that an entity should be hidden in the UI
 *
 * This class represents the attribute of all hidden items. The hidden
 * items shouldn't be displayed in UI applications (unless in some kind
 * of "debug" mode).
 *
 * @see Akonadi::Attribute
 *
 * @author Szymon Stefanek <s.stefanek@gmail.com>
 * @since 4.4
 */
class AKONADI_EXPORT EntityHiddenAttribute : public Attribute
{
  public:
    /**
     * Creates a new entity hidden attribute.
     */
    EntityHiddenAttribute();

    /**
     * Destroys the entity hidden attribute.
     */
    ~EntityHiddenAttribute();

    /**
     * Reimplemented from Attribute
     */
    QByteArray type() const;

    /**
     * Reimplemented from Attribute
     */
    EntityHiddenAttribute* clone() const;

    /**
     * Reimplemented from Attribute
     */
    QByteArray serialized() const;

    /**
     * Reimplemented from Attribute
     */
    void deserialize( const QByteArray &data );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
