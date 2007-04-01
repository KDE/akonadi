/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "collectionattributefactory.h"

#include <QHash>

using namespace Akonadi;

class CollectionAttributeFactory::Private
{
  public:
    QHash<QByteArray, CollectionAttribute*> attributes;
};

CollectionAttributeFactory* CollectionAttributeFactory::mInstance = 0;

CollectionAttributeFactory* CollectionAttributeFactory::self()
{
  if ( !mInstance ) {
    mInstance = new CollectionAttributeFactory();

    // register built-in attributes
    registerAttribute<CollectionContentTypeAttribute>();
  }
  return mInstance;
}

CollectionAttributeFactory::CollectionAttributeFactory()
  : d( new Private )
{
}

CollectionAttributeFactory::~ CollectionAttributeFactory()
{
  qDeleteAll( d->attributes );
  delete d;
}

void CollectionAttributeFactory::registerAttribute(CollectionAttribute *attr)
{
  Q_ASSERT( !d->attributes.contains( attr->type() ) );
  d->attributes.insert( attr->type(), attr );
}

CollectionAttribute* CollectionAttributeFactory::createAttribute(const QByteArray &type)
{
  CollectionAttribute* attr = self()->d->attributes.value( type );
  if ( attr )
    return attr->clone();
  return 0;
}
