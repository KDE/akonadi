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

#include "attributefactory.h"

#include "collectionrightsattribute.h"

#include <QtCore/QHash>

using namespace Akonadi;

class DefaultAttribute : public Attribute
{
  public:
    explicit DefaultAttribute( const QByteArray &type, const QByteArray &value = QByteArray() ) :
      mType( type ),
      mValue( value )
    {}

    QByteArray type() const { return mType; }
    Attribute* clone() const
    {
      return new DefaultAttribute( mType, mValue );
    }

    QByteArray serialized() const { return mValue; }
    void deserialize( const QByteArray &data ) { mValue = data; }

  private:
    QByteArray mType, mValue;
};

class AttributeFactory::Private
{
  public:
    QHash<QByteArray, Attribute*> attributes;
    static AttributeFactory* mInstance;
};

AttributeFactory* AttributeFactory::Private::mInstance = 0;

AttributeFactory* AttributeFactory::self()
{
  if ( !Private::mInstance ) {
    Private::mInstance = new AttributeFactory();

    // Register built-in attributes
    AttributeFactory::registerAttribute<CollectionRightsAttribute>();
  }

  return Private::mInstance;
}

AttributeFactory::AttributeFactory()
  : d( new Private )
{
}

AttributeFactory::~ AttributeFactory()
{
  qDeleteAll( d->attributes );
  delete d;
}

void AttributeFactory::registerAttribute(Attribute *attr)
{
  Q_ASSERT( !d->attributes.contains( attr->type() ) );
  d->attributes.insert( attr->type(), attr );
}

Attribute* AttributeFactory::createAttribute(const QByteArray &type)
{
  Attribute* attr = self()->d->attributes.value( type );
  if ( attr )
    return attr->clone();
  return new DefaultAttribute( type );
}
