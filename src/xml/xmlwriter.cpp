/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>

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

#include "xmlwriter.h"
#include "format_p.h"

#include "attribute.h"
#include "collection.h"
#include "item.h"


using namespace Akonadi;

QDomElement XmlWriter::attributeToElement(Attribute* attr, QDomDocument& document)
{
  if ( document.isNull() )
    return QDomElement();

  QDomElement top = document.createElement( Format::Tag::attribute() );
  top.setAttribute( Format::Attr::attributeType(), QString::fromUtf8( attr->type() ) );
  QDomText attrText = document.createTextNode( QString::fromUtf8( attr->serialized() ) );
  top.appendChild( attrText );

  return top;
}

void XmlWriter::writeAttributes(const Entity& entity, QDomElement& parentElem)
{
  if ( parentElem.isNull() )
    return;

  QDomDocument doc = parentElem.ownerDocument();
  foreach ( Attribute *attr, entity.attributes() )
    parentElem.appendChild( attributeToElement( attr, doc ) );
}

QDomElement XmlWriter::collectionToElement(const Akonadi::Collection& collection, QDomDocument& document)
{
  if ( document.isNull() )
    return QDomElement();

  QDomElement top = document.createElement( Format::Tag::collection() );
  top.setAttribute( Format::Attr::remoteId(), collection.remoteId() );
  top.setAttribute( Format::Attr::collectionName(), collection.name() );
  top.setAttribute( Format::Attr::collectionContentTypes(), collection.contentMimeTypes().join( QStringLiteral(",") ) );
  writeAttributes( collection, top );

  return top;
}

QDomElement XmlWriter::writeCollection(const Akonadi::Collection& collection, QDomElement& parentElem)
{
  if ( parentElem.isNull() )
    return QDomElement();

  QDomDocument doc = parentElem.ownerDocument();
  const QDomElement elem = collectionToElement( collection, doc );
  parentElem.insertBefore( elem, QDomNode() ); // collection need to be before items to pass schema validation
  return elem;
}

QDomElement XmlWriter::itemToElement(const Akonadi::Item& item, QDomDocument& document)
{
  if ( document.isNull() )
    return QDomElement();

  QDomElement top = document.createElement( Format::Tag::item() );
  top.setAttribute( Format::Attr::remoteId(), item.remoteId() );
  top.setAttribute( Format::Attr::itemMimeType(), item.mimeType() );

  if ( item.hasPayload() ) {
    QDomElement payloadElem = document.createElement( Format::Tag::payload() );
    QDomText payloadText = document.createTextNode( QString::fromUtf8( item.payloadData() ) );
    payloadElem.appendChild( payloadText );
    top.appendChild( payloadElem );
  }

  writeAttributes( item, top );

  foreach ( const Item::Flag &flag, item.flags() ) {
    QDomElement flagElem = document.createElement( Format::Tag::flag() );
    QDomText flagText = document.createTextNode( QString::fromUtf8( flag ) );
    flagElem.appendChild( flagText );
    top.appendChild( flagElem );
  }

  return top;
}

QDomElement XmlWriter::writeItem(const Item& item, QDomElement& parentElem)
{
  if ( parentElem.isNull() )
    return QDomElement();

  QDomDocument doc = parentElem.ownerDocument();
  const QDomElement elem = itemToElement( item, doc );
  parentElem.appendChild( elem );
  return elem;
}

