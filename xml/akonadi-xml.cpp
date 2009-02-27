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

#include <stdlib.h>

#include <akonadi/collection.h>

#include <QFileSystemWatcher>
#include <QtCore/QDir>
#include <QtCore/QUuid>
#include <QtXml/QDomElement>

using namespace Akonadi;

class GenericAttribute : public Attribute
{
  public:
    explicit GenericAttribute( const QByteArray &type, const QByteArray &value = QByteArray() ) :
      mType( type ),
      mValue( value )
    {}

    QByteArray type() const { return mType; }
    Attribute* clone() const
    {
      return new GenericAttribute( mType, mValue );
    }

    QByteArray serialized() const { return mValue; }
    void deserialize( const QByteArray &data ) { mValue = data; }

  private:
    QByteArray mType, mValue;
};

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

void AkonadiXML::deserializeAttributes( const QDomElement &node, Entity &entity )
{
  const QDomNodeList children = node.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement attrElem = children.at( i ).toElement();
    if ( attrElem.isNull() || attrElem.tagName() != "attribute" )
      continue;
    GenericAttribute *attr = new GenericAttribute( attrElem.attribute( "type" ).toUtf8(),
                                                   attrElem.text().toUtf8() );
    entity.addAttribute( attr );
  }
}

Collection AkonadiXML::buildCollection( const QDomElement &node, const QString &parentRid )
{
  Collection c;
  c.setRemoteId( node.attribute( "rid" ) );
  c.setParentRemoteId( parentRid );
  c.setName( node.attribute( "name" ) );
  c.setContentMimeTypes( node.attribute( "content" ).split( ',' ) );
  deserializeAttributes( node, c );
  return c;
}

Collection::List AkonadiXML::buildCollectionTree( const QDomElement &parent )
{
  Collection::List rv;
  if ( parent.isNull() )
    return rv;
  const QDomNodeList children = parent.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement child = children.at( i ).toElement();
    if ( child.isNull() || child.tagName() != "collection" )
      continue;
    rv += buildCollection( child, parent.attribute( "rid" ) );
    rv += buildCollectionTree( child );
  }
  return rv;
}

void addItemPayload( Item &item, const QDomElement &elem )
{
  if ( elem.isNull() )
    return;
  const QByteArray payloadData = elem.text().toUtf8();
  item.setPayloadFromData( payloadData );
}

Item AkonadiXML::buildItem( const QDomElement &elem )
{
  Item item( elem.attribute( "mimetype", "application/octet-stream" ) );
  item.setRemoteId( elem.attribute( "rid" ) );
  deserializeAttributes( elem, item );

  const QDomNodeList children = elem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement flagElem = children.at( i ).toElement();
    if ( flagElem.isNull() || flagElem.tagName() != "flag" )
      continue;
    item.setFlag( flagElem.text().toUtf8() );
  }

  return item;
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
