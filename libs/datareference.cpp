/*
    This file is part of libakonadi.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include <QtCore/QHash>
#include <QtCore/QSharedData>

#include "datareference.h"

using namespace Akonadi;

class DataReference::Private : public QSharedData
{
  public:
    Private()
      : mId( -1 )
    {
    }

    Private( const Private &other )
      : QSharedData( other )
    {
      mId = other.mId;
      mRemoteId = other.mRemoteId;
    }

    int mId;
    QString mRemoteId;
};

DataReference::DataReference()
  : d( new Private )
{
}

DataReference::DataReference( int id, const QString &remoteId )
  : d( new Private )
{
  d->mId = id;
  setRemoteId ( remoteId );
}

DataReference::DataReference( const DataReference &other )
  : d( other.d )
{
}

DataReference::~DataReference()
{
}

DataReference& DataReference::operator=( const DataReference &other )
{
  if ( this != &other )
    d = other.d;

  return *this;
}

int DataReference::id() const
{
  return d->mId;
}

QString DataReference::remoteId() const
{
  return d->mRemoteId;
}

void DataReference::setRemoteId( const QString &id )
{
  d->mRemoteId=id;
}

bool DataReference::isNull() const
{
  return d->mId < 0;
}

bool DataReference::operator==( const DataReference & other ) const
{
  return d->mId == other.d->mId;
}

bool DataReference::operator !=( const DataReference & other ) const
{
  return !(*this == other);
}

bool DataReference::operator<( const DataReference & other ) const
{
  return d->mId < other.d->mId;
}

uint qHash( const DataReference& reference )
{
  return qHash( reference.id() );
}


