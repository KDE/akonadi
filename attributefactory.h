/*
    Copyright (c) 2007 - 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ATTRIBUTEFACTORY_H
#define AKONADI_ATTRIBUTEFACTORY_H

#include <akonadi/attribute.h>

namespace Akonadi {

/**
 * @short The AttributeFactory class provides the functionality of registering and creating arbitrary
 *        entity attributes.
 *
 * This class provides the functionality of registering and creating arbitrary Attributes for Entity
 * and its subclasses (e.g. Item and Collection).
 */
class AKONADI_EXPORT AttributeFactory
{
  public:
    /**
     * Register a custom attribute of type T.
     */
    template <typename T> inline static void registerAttribute()
    {
      AttributeFactory::self()->registerAttribute( new T );
    }

    /**
     * Creates a collection attribute object of the given type.
     *
     * @param type The attribute type.
     */
    static Attribute* createAttribute( const QByteArray &type );

  private:
    /**
     * Creates a new attribute factory.
     */
    AttributeFactory();

    /**
     * Destroys the attribute factory.
     */
    ~AttributeFactory();

    /**
     * Returns the global instance of the attribute factory.
     */
    static AttributeFactory* self();

    /**
     * Registers a new @p attribute to the factory.
     */
    void registerAttribute( Attribute *attribute );

  private:
    class Private;
    Private* const d;
};

}

#endif
