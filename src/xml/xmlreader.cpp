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

#include "xmlreader.h"
#include "format_p.h"

#include "attributefactory.h"


using namespace Akonadi;

Attribute* XmlReader::elementToAttribute(const QDomElement& elem)
{
  if ( elem.isNull() || elem.tagName() != Format::Tag::attribute() )
    return 0;
  Attribute *attr = AttributeFactory::createAttribute( elem.attribute( Format::Attr::attributeType() ).toUtf8() );
  Q_ASSERT( attr );
  attr->deserialize( elem.text().toUtf8() );
  return attr;
}

void XmlReader::readAttributes(const QDomElement& elem, Entity& entity)
{
  if ( elem.isNull() )
    return;
  const QDomNodeList children = elem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement attrElem = children.at( i ).toElement();
    Attribute *attr = elementToAttribute( attrElem );
    if ( attr )
      entity.addAttribute( attr );
  }
}

Collection XmlReader::elementToCollection(const QDomElement& elem)
{
  if ( elem.isNull() || elem.tagName() != Format::Tag::collection() )
    return Collection();

  Collection c;
  c.setRemoteId( elem.attribute( Format::Attr::remoteId() ) );
  c.setName( elem.attribute( Format::Attr::collectionName() ) );
  c.setContentMimeTypes( elem.attribute( Format::Attr::collectionContentTypes() ).split( QLatin1Char(',') ) );
  XmlReader::readAttributes( elem, c );

  const QDomElement parentElem = elem.parentNode().toElement();
  if ( !parentElem.isNull() && parentElem.tagName() == Format::Tag::collection() )
    c.parentCollection().setRemoteId( parentElem.attribute( Format::Attr::remoteId() ) );

  return c;
}

Collection::List XmlReader::readCollections(const QDomElement& elem)
{
  Collection::List rv;
  if ( elem.isNull() )
    return rv;
  if ( elem.tagName() == Format::Tag::collection() )
    rv += elementToCollection( elem );
  const QDomNodeList children = elem.childNodes();
  for ( int i = 0; i < children.count(); i++ ) {
    const QDomElement child = children.at( i ).toElement();
    if ( child.isNull() || child.tagName() != Format::Tag::collection() )
      continue;
    rv += readCollections( child );
  }
  return rv;
}

Item XmlReader::elementToItem(const QDomElement& elem, bool includePayload)
{
  Item item( elem.attribute( Format::Attr::itemMimeType(), QStringLiteral("application/octet-stream") ) );
  item.setRemoteId( elem.attribute( Format::Attr::remoteId() ) );
  XmlReader::readAttributes( elem, item );
  
  const QDomNodeList children = elem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement subElem = children.at( i ).toElement();
    if ( subElem.isNull() )
      continue;
    if ( subElem.tagName() == Format::Tag::flag() ) {
      item.setFlag( subElem.text().toUtf8() );
    } else if ( includePayload && subElem.tagName() == Format::Tag::payload() ) {
      const QByteArray payloadData = subElem.text().toUtf8();
      item.setPayloadFromData( payloadData );
    }
  }

  return item;
}
