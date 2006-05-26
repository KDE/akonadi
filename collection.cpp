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
    QStringList contentTypes;
};

PIM::Collection::Collection( const QByteArray &path ) :
  d( new Collection::Private() )
{
  d->path = path;
  d->type = Unknown;
}

PIM::Collection::Collection( const Collection &other ) :
    d( new Collection::Private() )
{
  d->name = other.name();
  d->path = other.path();
  d->parent = other.parent();
  d->type = other.type();
  d->contentTypes = other.contentTypes();
}

PIM::Collection::~Collection( )
{
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

QStringList PIM::Collection::contentTypes( ) const
{
  return d->contentTypes;
}

void PIM::Collection::setContentTypes( const QStringList & types )
{
  d->contentTypes = types;
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
