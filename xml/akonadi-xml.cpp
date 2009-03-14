/*
    Copyright (c) Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
    based on  kdepim/akonadi/resources/knut

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

#include "akonadi-xml.h"
#include "genericattribute_p.h"
#include "xmlreader.h"

#include <stdlib.h>

#include <akonadi/collection.h>

#include <QFileSystemWatcher>
#include <QtCore/QDir>
#include <QtCore/QUuid>
#include <QtXml/QDomElement>

using namespace Akonadi;

void AkonadiXML::serializeAttributes( const Akonadi::Entity &entity, QDomElement &entityElem )
{
  foreach ( Attribute *attr, entity.attributes() ) {
    QDomElement top = mDocument.createElement( "attribute" );
    top.setAttribute( "type", QString::fromUtf8( attr->type() ) );
    QDomText attrText = mDocument.createTextNode( QString::fromUtf8( attr->serialized() ) );
    top.appendChild( attrText );
    entityElem.appendChild( top );
  }
}

Collection::List AkonadiXML::buildCollectionTree( const QDomElement &parent )
{
  Collection::List rv;
  if ( parent.isNull() )
    return rv;
  const QDomNodeList children = parent.childNodes();
  for ( int i = 0; i < children.count(); i++ ) {
    const QDomElement child = children.at( i ).toElement();
    if ( child.isNull() || child.tagName() != "collection" )
      continue;
    rv += XmlReader::elementToCollection( child );
    rv += buildCollectionTree( child );
  }
  return rv;
}

QDomElement AkonadiXML::serializeItem( Akonadi::Item &item )
{
  if ( item.remoteId().isEmpty() )
    item.setRemoteId( QUuid::createUuid().toString() );
  QDomElement top = mDocument.createElement( "item" );
  top.setAttribute( "rid", item.remoteId() );
  top.setAttribute( "mimetype", item.mimeType() );

  QDomElement payloadElem = mDocument.createElement( "payload" );
  QDomText payloadText = mDocument.createTextNode( QString::fromUtf8( item.payloadData() ) );
  payloadElem.appendChild( payloadText );
  top.appendChild( payloadElem );

  serializeAttributes( item, top );

  foreach ( const Item::Flag &flag, item.flags() ) {
    QDomElement flagElem = mDocument.createElement( "flag" );
    QDomText flagText = mDocument.createTextNode( QString::fromUtf8( flag ) );
    flagElem.appendChild( flagText );
    top.appendChild( flagElem );
  }

  return top;
}

QDomElement AkonadiXML::serializeCollection( Akonadi::Collection &collection )
{
  if ( collection.remoteId().isEmpty() )
    collection.setRemoteId( QUuid::createUuid().toString() );
  QDomElement top = mDocument.createElement( "collection" );
  top.setAttribute( "rid", collection.remoteId() );
  top.setAttribute( "name", collection.name() );
  top.setAttribute( "content", collection.contentMimeTypes().join( "," ) );
  serializeAttributes( collection, top );
  return top;
}
