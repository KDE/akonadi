/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ATTRIBUTE_H
#define AKONADI_ATTRIBUTE_H

#include "akonadi_export.h"

#include <QtCore/QList>

namespace Akonadi {

/**
 * Stores specific entity attributes (ACLs, quotas, etc.).
 * Every entity can have zero ore one attribute of every type.
 */
class AKONADI_EXPORT Attribute
{
  public:
    /**
     * Describes a list of attributes.
     */
    typedef QList<Attribute*> List;

    /**
     * Returns the type of the attribute.
     */
    virtual QByteArray type() const = 0;

    /**
     * Destroys this attribute.
     */
    virtual ~Attribute();

    /**
     * Creates a copy of this attribute.
     */
    virtual Attribute* clone() const = 0;

    /**
     * Returns a QByteArray representation of the attribute which will be
     * storaged. This can be raw binary data, no encoding needs to be applied.
     */
    virtual QByteArray serialized() const = 0;

    /**
     * Sets the data of this attribute, using the same encoding
     * as returned by toByteArray().
     *
     * @param data The encoded attribute data.
     */
    virtual void deserialize( const QByteArray &data ) = 0;
};

}

#endif
