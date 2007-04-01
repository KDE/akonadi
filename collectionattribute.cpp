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

#include "collectionattribute.h"
#include "imapparser.h"

using namespace Akonadi;

CollectionAttribute::~ CollectionAttribute( )
{
}

CollectionContentTypeAttribute::CollectionContentTypeAttribute( const QList< QByteArray > & contentTypes )
{
  mContentTypes = contentTypes;
}

QByteArray CollectionContentTypeAttribute::type() const
{
  return QByteArray( "MIMETYPE" );
}

QList< QByteArray > CollectionContentTypeAttribute::contentTypes( ) const
{
  return mContentTypes;
}

void CollectionContentTypeAttribute::setContentTypes( const QList< QByteArray > & contentTypes )
{
  mContentTypes = contentTypes;
}

CollectionContentTypeAttribute * CollectionContentTypeAttribute::clone() const
{
  return new CollectionContentTypeAttribute( mContentTypes );
}

QByteArray CollectionContentTypeAttribute::toByteArray() const
{
  return '(' + ImapParser::join( contentTypes(), " " ) + ')';
}

void CollectionContentTypeAttribute::setData(const QByteArray & data)
{
  QList<QByteArray> list;
  ImapParser::parseParenthesizedList( data, list );
  setContentTypes( list );
}
