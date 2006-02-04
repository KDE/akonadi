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

class Collection::CollectionPrivate
{
  public:
    DataReference ref;
    DataReference parent;
    QString name;
    Type type;
    QStringList contentTypes;
    QString query;
};

PIM::Collection::Collection( const DataReference &ref ) :
  d( new Collection::CollectionPrivate() )
{
  d->ref = ref;
  d->type = Unknown;
}

PIM::Collection::~ Collection( )
{
  delete d;
  d = 0;
}

DataReference PIM::Collection::reference( ) const
{
  return d->ref;
}

QString PIM::Collection::name( ) const
{
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

DataReference PIM::Collection::parent( ) const
{
  return d->parent;
}

void PIM::Collection::setParent( const DataReference & parent )
{
  d->parent = parent;
}

QString PIM::Collection::query( ) const
{
  return d->query;
}

void PIM::Collection::setQuery( const QString & query )
{
  d->query = query;
}

void PIM::Collection::copy( Collection * col )
{
  d->name = col->name();
  d->ref = col->reference();
  d->parent = col->parent();
  d->type = col->type();
  d->contentTypes = col->contentTypes();
  d->query = col->query();
}
