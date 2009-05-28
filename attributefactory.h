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
 * @short Provides the functionality of registering and creating arbitrary
 *        entity attributes.
 *
 * This class provides the functionality of registering and creating arbitrary Attributes for Entity
 * and its subclasses (e.g. Item and Collection).
 *
 * @code
 *
 * // register the type first
 * Akonadi::AttributeFactory::registerAttribute<SecrecyAttribute>();
 *
 * ...
 *
 * // use it anywhere else in the application
 * SecrecyAttribute *attr = Akonadi::AttributeFactory::createAttribute( "secrecy" );
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT AttributeFactory
{
  public:
    //@cond PRIVATE
    ~AttributeFactory();
    //@endcond

    /**
     * Registers a custom attribute of type T.
     * The same attribute cannot be registered more than once.
     */
    template <typename T> inline static void registerAttribute()
    {
      AttributeFactory::self()->registerAttribute( new T );
    }

    /**
     * Creates an entity attribute object of the given type.
     * If the type has not been registered, creates a DefaultAttribute.
     *
     * @param type The attribute type.
     */
    static Attribute* createAttribute( const QByteArray &type );

  protected:
    //@cond PRIVATE
    AttributeFactory();

  private:
    static AttributeFactory* self();
    void registerAttribute( Attribute *attribute );

    class Private;
    Private* const d;
    //@endcond
};

}

#endif
