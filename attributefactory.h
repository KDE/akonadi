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
  Factory for collection attributes.
*/
class AKONADI_EXPORT AttributeFactory
{
  public:
    /**
      Register a custom attribute.
    */
    template <typename T> inline static void registerAttribute()
    {
      AttributeFactory::self()->registerAttribute( new T );
    }

    /**
      Creates a collection attribute object of the given type.
      @param type The attribute type.
    */
    static Attribute* createAttribute( const QByteArray &type );

  private:
    AttributeFactory();
    ~AttributeFactory();
    static AttributeFactory* self();
    void registerAttribute( Attribute *attr );

  private:
    class Private;
    Private* const d;
};

}

#endif
