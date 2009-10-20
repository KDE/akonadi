/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "specialcollectionattribute_p.h"
#include "specialcollections.h"

#include <KDebug>

#include <akonadi/attributefactory.h>

using namespace Akonadi;

/**
  @internal
*/
class SpecialCollectionAttribute::Private
{
  public:
    SpecialCollections::Type mType;
};

SpecialCollectionAttribute::SpecialCollectionAttribute( SpecialCollections::Type type )
  : d( new Private )
{
  d->mType = type;
}

SpecialCollectionAttribute::~SpecialCollectionAttribute()
{
  delete d;
}

SpecialCollectionAttribute* SpecialCollectionAttribute::clone() const
{
  return new SpecialCollectionAttribute( d->mType );
}

QByteArray SpecialCollectionAttribute::type() const
{
  static const QByteArray sType( "SpecialCollectionAttribute" );
  return sType;
}

QByteArray SpecialCollectionAttribute::serialized() const
{
  return QByteArray::number( (int)d->mType );
}

void SpecialCollectionAttribute::deserialize( const QByteArray &data )
{
  d->mType = (SpecialCollections::Type)data.toInt();
}

void SpecialCollectionAttribute::setCollectionType( SpecialCollections::Type type )
{
    Q_ASSERT( type > SpecialCollections::Invalid && type < SpecialCollections::LastType );
    d->mType = type;
  }

SpecialCollections::Type SpecialCollectionAttribute::collectionType() const
{
  return d->mType;
}

// Register the attribute when the library is loaded.
namespace {

bool dummySpecialCollectionAttribute()
{
  using namespace Akonadi;
  AttributeFactory::registerAttribute<SpecialCollectionAttribute>();
  return true;
}

const bool registeredSpecialCollectionAttribute = dummySpecialCollectionAttribute();

}
