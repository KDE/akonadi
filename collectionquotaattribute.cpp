/*
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

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

#include "collectionquotaattribute.h"

#include <QtCore/QByteArray>

using namespace Akonadi;

CollectionQuotaAttribute::CollectionQuotaAttribute()
  : mCurrent( -1 ), mMax( -1 )
{
}

CollectionQuotaAttribute::CollectionQuotaAttribute( qint64 currentValue, qint64 maxValue )
  : mCurrent( currentValue ), mMax( maxValue )
{
}

void CollectionQuotaAttribute::setValues( qint64 currentValue, qint64 maxValue )
{
  mCurrent = currentValue;
  mMax = maxValue;
}

qint64 CollectionQuotaAttribute::currentValue() const
{
  return mCurrent;
}

qint64 CollectionQuotaAttribute::maxValue() const
{
  return mMax;
}

QByteArray CollectionQuotaAttribute::type() const
{
  return "collectionquota";
}

Akonadi::Attribute* CollectionQuotaAttribute::clone() const
{
  return new CollectionQuotaAttribute( mCurrent, mMax );
}

QByteArray CollectionQuotaAttribute::serialized() const
{
  return QByteArray::number( mCurrent )
       + ' '
       + QByteArray::number( mMax );
}

void CollectionQuotaAttribute::deserialize( const QByteArray &data )
{
  mCurrent = -1;
  mMax = -1;

  QList<QByteArray> items = data.simplified().split( ' ' );

  if ( items.isEmpty() ) {
    return;
  }

  mCurrent = items[0].toLongLong();

  if ( items.size() < 2 ) {
    return;
  }

  mMax = items[1].toLongLong();
}
