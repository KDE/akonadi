/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collection.h"

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

using namespace Akonadi;

class Collection::Private
{
  public:
    QString path;
    QString parent;
    QString name;
    Type type;
    QHash<QByteArray, CollectionAttribute*> attributes;
};

Collection::Collection( const QString &path ) :
  d( new Collection::Private() )
{
  d->path = path;
  d->type = Unknown;
}

Collection::~Collection( )
{
  qDeleteAll( d->attributes );
  delete d;
}

QString Collection::path() const
{
  return d->path;
}

QString Collection::name( ) const
{
  if ( d->name.isEmpty() && d->path != root() ) {
    QString name = d->path.mid( d->path.lastIndexOf( delimiter() ) + 1 );
    d->name = name;
  }
  return d->name;
}

void Collection::setName( const QString & name )
{
  d->name = name;
}

Collection::Type Collection::type() const
{
  return d->type;
}

void Collection::setType( Type type )
{
  d->type = type;
}

QList<QByteArray> Collection::contentTypes() const
{
  CollectionContentTypeAttribute *attr = const_cast<Collection*>( this )->attribute<CollectionContentTypeAttribute>();
  if ( attr )
    return attr->contentTypes();
  return QList<QByteArray>();
}

void Collection::setContentTypes( const QList<QByteArray> & types )
{
  CollectionContentTypeAttribute* attr = attribute<CollectionContentTypeAttribute>( true );
  attr->setContentTypes( types );
}

QString Collection::parent( ) const
{
  return d->parent;
}

void Collection::setParent( const QString &parent )
{
  d->parent = parent;
}

QString Collection::delimiter()
{
  return QLatin1String( "/" );
}

QString Collection::root( )
{
  return QString();
}

QString Collection::searchFolder( )
{
  return root() + QLatin1String( "Search" );
}

QString Collection::prefix()
{
  return QLatin1String( "/" );
}

QByteArray Collection::collectionMimeType( )
{
  return QByteArray( "inode/directory" );
}

void Collection::addAttribute( CollectionAttribute * attr )
{
  if ( d->attributes.contains( attr->type() ) )
    delete d->attributes.take( attr->type() );
  d->attributes.insert( attr->type(), attr );
}

CollectionAttribute * Collection::attribute( const QByteArray & type ) const
{
  if ( d->attributes.contains( type ) )
    return d->attributes.value( type );
  return 0;
}

bool Collection::hasAttribute( const QByteArray & type ) const
{
  return d->attributes.contains( type );
}
