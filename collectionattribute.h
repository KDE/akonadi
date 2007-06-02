/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONATTRIBUTE_H
#define AKONADI_COLLECTIONATTRIBUTE_H

#include "libakonadi_export.h"
#include <QtCore/QList>
#include <QtCore/QStringList>

class QString;

namespace Akonadi {

/**
  Stores specific collection attributes (ACLs, unread counts, quotas, etc.).
  Every collection can have zero ore one attribute of every type.
*/
class AKONADI_EXPORT CollectionAttribute
{
  public:
    typedef QList<CollectionAttribute*> List;

    /**
      Returns the attribute name of this collection.
    */
    virtual QByteArray type() const = 0;

    /**
      Destroys this attribute.
    */
    virtual ~CollectionAttribute();

    /**
      Creates a copy of this object.
    */
    virtual CollectionAttribute* clone() const = 0;

    /**
      Returns a QByteArray representation of the attribute which will be
      storaged. This can be raw binary data, no encoding needs to be applied.
    */
    virtual QByteArray toByteArray() const = 0;

    /**
      Sets the data of this attribute, using the same encoding
      as returned by toByteArray().
      @param data The encoded attribute data.
    */
    virtual void setData( const QByteArray &data ) = 0;
};

}

#endif
