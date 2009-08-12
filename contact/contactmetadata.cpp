/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "contactmetadata_p.h"

#include "contactmetadataattribute_p.h"

#include <akonadi/item.h>

using namespace Akonadi;

class ContactMetaData::Private
{
  public:
    int mDisplayNameMode;
};

ContactMetaData::ContactMetaData()
  : d( new Private )
{
}

ContactMetaData::~ContactMetaData()
{
  delete d;
}

void ContactMetaData::load( const Akonadi::Item &contact )
{
  if ( !contact.hasAttribute( "contactmetadata" ) )
    return;

  ContactMetaDataAttribute *attribute = contact.attribute<ContactMetaDataAttribute>();
  const QVariantMap metaData = attribute->metaData();

  d->mDisplayNameMode = metaData.value( QLatin1String( "DisplayNameMode" ) ).toInt();
}

void ContactMetaData::store( Akonadi::Item &contact )
{
  ContactMetaDataAttribute *attribute = contact.attribute<ContactMetaDataAttribute>( Item::AddIfMissing );

  QVariantMap metaData;
  metaData.insert( QLatin1String( "DisplayNameMode" ), QVariant( d->mDisplayNameMode ) );

  attribute->setMetaData( metaData );
}

void ContactMetaData::setDisplayNameMode( int mode )
{
  d->mDisplayNameMode = mode;
}

int ContactMetaData::displayNameMode() const
{
  return d->mDisplayNameMode;
}
