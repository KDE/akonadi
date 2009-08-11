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

#include "addressattribute.h"

#include <QDataStream>

#include <KDebug>

#include <akonadi/attributefactory.h>

using namespace Akonadi;

/**
  @internal
*/
class AddressAttribute::Private
{
  public:
    QString mFrom;
    QStringList mTo;
    QStringList mCc;
    QStringList mBcc;
};

AddressAttribute::AddressAttribute( const QString &from, const QStringList &to,
                                    const QStringList &cc, const QStringList &bcc )
  : d( new Private )
{
  d->mFrom = from;
  d->mTo = to;
  d->mCc = cc;
  d->mBcc = bcc;
}

AddressAttribute::~AddressAttribute()
{
  delete d;
}

AddressAttribute* AddressAttribute::clone() const
{
  return new AddressAttribute( d->mFrom, d->mTo, d->mCc, d->mBcc );
}

QByteArray AddressAttribute::type() const
{
  static const QByteArray sType( "AddressAttribute" );
  return sType;
}

QByteArray AddressAttribute::serialized() const
{
  QByteArray serializedData;
  QDataStream serializer( &serializedData, QIODevice::WriteOnly );
  serializer.setVersion( QDataStream::Qt_4_5 );
  serializer << d->mFrom;
  serializer << d->mTo;
  serializer << d->mCc;
  serializer << d->mBcc;
  return serializedData;
}

void AddressAttribute::deserialize( const QByteArray &data )
{
  QDataStream deserializer( data );
  deserializer.setVersion( QDataStream::Qt_4_5 );
  deserializer >> d->mFrom;
  deserializer >> d->mTo;
  deserializer >> d->mCc;
  deserializer >> d->mBcc;
}

QString AddressAttribute::from() const
{
  return d->mFrom;
}

void AddressAttribute::setFrom( const QString &from )
{
  d->mFrom = from;
}

QStringList AddressAttribute::to() const
{
  return d->mTo;
}

void AddressAttribute::setTo( const QStringList &to )
{
  d->mTo = to;
}

QStringList AddressAttribute::cc() const
{
  return d->mCc;
}

void AddressAttribute::setCc( const QStringList &cc )
{
  d->mCc = cc;
}

QStringList AddressAttribute::bcc() const
{
  return d->mBcc;
}

void AddressAttribute::setBcc( const QStringList &bcc )
{
  d->mBcc = bcc;
}

// Register the attribute when the library is loaded.
namespace {

bool dummy()
{
  using namespace Akonadi;
  AttributeFactory::registerAttribute<AddressAttribute>();
  return true;
}

const bool registered = dummy();

}
