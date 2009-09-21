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

#include "localfolderattribute.h"
#include "localfolders.h"

#include <KDebug>

#include <akonadi/attributefactory.h>

using namespace Akonadi;

/**
  @internal
*/
class LocalFolderAttribute::Private
{
  public:
    int type;
};

LocalFolderAttribute::LocalFolderAttribute( int type )
  : d( new Private )
{
  d->type = type;
}

LocalFolderAttribute::~LocalFolderAttribute()
{
  delete d;
}

LocalFolderAttribute* LocalFolderAttribute::clone() const
{
  return new LocalFolderAttribute( d->type );
}

QByteArray LocalFolderAttribute::type() const
{
  static const QByteArray sType( "LocalFolderAttribute" );
  return sType;
}

QByteArray LocalFolderAttribute::serialized() const
{
  return QByteArray::number( d->type );
}

void LocalFolderAttribute::deserialize( const QByteArray &data )
{
  d->type = data.toInt();
}

int LocalFolderAttribute::folderType() const
{
  return d->type;
}

void LocalFolderAttribute::setFolderType( int type )
{
  Q_ASSERT( type >= 0 && type < LocalFolders::LastDefaultType );
  d->type = type;
}

// Register the attribute when the library is loaded.
namespace {

bool dummyLocalFolderAttribute()
{
  using namespace Akonadi;
  AttributeFactory::registerAttribute<LocalFolderAttribute>();
  return true;
}

const bool registeredLocalFolderAttribute = dummyLocalFolderAttribute();

}
