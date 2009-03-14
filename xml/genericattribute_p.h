/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_XML_GENERICATTRIBUTE_P_H
#define AKONADI_XML_GENERICATTRIBUTE_P_H

#include <akonadi/attribute.h>

namespace Akonadi {

class GenericAttribute : public Attribute
{
  public:
    explicit GenericAttribute( const QByteArray &type, const QByteArray &value = QByteArray() ) :
    mType( type ),
    mValue( value )
    {}

    QByteArray type() const { return mType; }
    Attribute* clone() const
    {
      return new GenericAttribute( mType, mValue );
    }

    QByteArray serialized() const { return mValue; }
    void deserialize( const QByteArray &data ) { mValue = data; }

  private:
    QByteArray mType, mValue;
};
  
}

#endif
