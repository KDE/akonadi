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

#include "contactmetadata_p.h"

#include "contactmetadataattribute_p.h"

#include <akonadi/item.h>

using namespace Akonadi;

class ContactMetaData::Private
{
  public:
    Private()
      : mDisplayNameMode( -1 )
    {
    }

    int mDisplayNameMode;
    QVariantList mCustomFieldDescriptions;
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
  if ( !contact.hasAttribute( "contactmetadata" ) ) {
    return;
  }

  ContactMetaDataAttribute *attribute = contact.attribute<ContactMetaDataAttribute>();
  const QVariantMap metaData = attribute->metaData();

  if ( metaData.contains( QLatin1String( "DisplayNameMode" ) ) ) {
    d->mDisplayNameMode = metaData.value( QLatin1String( "DisplayNameMode" ) ).toInt();
  } else {
    d->mDisplayNameMode = -1;
  }

  d->mCustomFieldDescriptions = metaData.value( QLatin1String( "CustomFieldDescriptions" ) ).toList();
}

void ContactMetaData::store( Akonadi::Item &contact )
{
  ContactMetaDataAttribute *attribute = contact.attribute<ContactMetaDataAttribute>( Item::AddIfMissing );

  QVariantMap metaData;
  if ( d->mDisplayNameMode != -1 ) {
    metaData.insert( QLatin1String( "DisplayNameMode" ), QVariant( d->mDisplayNameMode ) );
  }

  if ( !d->mCustomFieldDescriptions.isEmpty() ) {
    metaData.insert( QLatin1String( "CustomFieldDescriptions" ), d->mCustomFieldDescriptions );
  }

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

void ContactMetaData::setCustomFieldDescriptions( const QVariantList &descriptions )
{
  d->mCustomFieldDescriptions = descriptions;
}

QVariantList ContactMetaData::customFieldDescriptions() const
{
  return d->mCustomFieldDescriptions;
}
