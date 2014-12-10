/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "contactmetadataattribute_p.h"

#include <QtCore/QDataStream>

using namespace Akonadi;

class ContactMetaDataAttribute::Private
{
  public:
    QVariantMap mData;
};

ContactMetaDataAttribute::ContactMetaDataAttribute()
  : d( new Private )
{
}

ContactMetaDataAttribute::~ContactMetaDataAttribute()
{
  delete d;
}

void ContactMetaDataAttribute::setMetaData( const QVariantMap &data )
{
  d->mData = data;
}

QVariantMap ContactMetaDataAttribute::metaData() const
{
  return d->mData;
}

QByteArray ContactMetaDataAttribute::type() const
{
    static const QByteArray sType( "contactmetadata" );
    return sType;
}

Attribute* ContactMetaDataAttribute::clone() const
{
  ContactMetaDataAttribute *copy = new ContactMetaDataAttribute;
  copy->setMetaData( d->mData );

  return copy;
}

QByteArray ContactMetaDataAttribute::serialized() const
{
  QByteArray data;
  QDataStream s( &data, QIODevice::WriteOnly );
  s.setVersion( QDataStream::Qt_4_5 );
  s << d->mData;

  return data;
}

void ContactMetaDataAttribute::deserialize( const QByteArray &data )
{
  QDataStream s( data );
  s.setVersion( QDataStream::Qt_4_5 );
  s >> d->mData;
}
