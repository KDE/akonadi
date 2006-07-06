/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include <QHash>
#include <QString>
#include <QStringList>

using namespace PIM;

class Collection::Private
{
  public:
    QByteArray path;
    QByteArray parent;
    QString name;
    Type type;
    QHash<QByteArray, CollectionAttribute*> attributes;
};

PIM::Collection::Collection( const QByteArray &path ) :
  d( new Collection::Private() )
{
  d->path = path;
  d->type = Unknown;
}

PIM::Collection::~Collection( )
{
  qDeleteAll( d->attributes );
  delete d;
  d = 0;
}

QByteArray PIM::Collection::path() const
{
  return d->path;
}

QString PIM::Collection::name( ) const
{
  if ( d->name.isEmpty() && d->path != root() ) {
    QByteArray name = d->path.mid( d->path.lastIndexOf( delimiter() ) + 1 );
    // TODO decode to utf-8
    d->name = QString::fromLatin1( name );
  }
  return d->name;
}

void PIM::Collection::setName( const QString & name )
{
  d->name = name;
}

Collection::Type PIM::Collection::type() const
{
  return d->type;
}

void PIM::Collection::setType( Type type )
{
  d->type = type;
}

QList<QByteArray> PIM::Collection::contentTypes() const
{
  CollectionContentTypeAttribute *attr = const_cast<Collection*>( this )->attribute<CollectionContentTypeAttribute>();
  if ( attr )
    return attr->contentTypes();
  return QList<QByteArray>();
}

void PIM::Collection::setContentTypes( const QList<QByteArray> & types )
{
  CollectionContentTypeAttribute* attr = attribute<CollectionContentTypeAttribute>( true );
  attr->setContentTypes( types );
}

QByteArray PIM::Collection::parent( ) const
{
  return d->parent;
}

void PIM::Collection::setParent( const QByteArray &parent )
{
  d->parent = parent;
}

QByteArray PIM::Collection::delimiter()
{
  return QByteArray( "/" );
}

QByteArray PIM::Collection::root( )
{
  return QByteArray();
}

QByteArray PIM::Collection::searchFolder( )
{
  return root() + QByteArray( "Search" );
}

QByteArray PIM::Collection::prefix()
{
  return QByteArray( "/" );
}

QByteArray PIM::Collection::collectionMimeType( )
{
  return QByteArray( "inode/directory" );
}

void PIM::Collection::addAttribute( CollectionAttribute * attr )
{
  if ( d->attributes.contains( attr->type() ) )
    delete d->attributes.take( attr->type() );
  d->attributes.insert( attr->type(), attr );
}

CollectionAttribute * PIM::Collection::attribute( const QByteArray & type ) const
{
  if ( d->attributes.contains( type ) )
    return d->attributes.value( type );
  return 0;
}

bool PIM::Collection::hasAttribute( const QByteArray & type ) const
{
  return d->attributes.contains( type );
}
