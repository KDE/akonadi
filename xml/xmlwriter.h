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

#ifndef AKONADI_XMLWRITER_H
#define AKONADI_XMLWRITER_H

#include "akonadi-xml_export.h"

#include <QtXml/QDomElement>

namespace Akonadi {

class Attribute;
class Collection;
class Entity;
class Item;

/**
  Low-level methods to serialize Akonadi objects into XML.
  @see Akonadi::XmlDocument
*/
namespace XmlWriter
{
  /**
    Creates an attribute element for the given document.
  */
  AKONADI_XML_EXPORT QDomElement attributeToElement( Attribute* attr, QDomDocument &document );

  /**
    Serializes all attributes of the given Akonadi object into the given parent element.
  */
  AKONADI_XML_EXPORT void writeAttributes( const Entity &entity, QDomElement &parentElem );

  /**
    Creates a collection element for the given document, not yet attached to the DOM tree.
  */
  AKONADI_XML_EXPORT QDomElement collectionToElement( const Collection &collection, QDomDocument &document );

  /**
    Serializes the given collection into a DOM element with the given parent.
  */
  AKONADI_XML_EXPORT QDomElement writeCollection( const Collection &collection, QDomElement &parentElem );

  /**
    Creates an item element for the given document, not yet attached to the DOM tree
  */
  AKONADI_XML_EXPORT QDomElement itemToElement( const Item &item, QDomDocument &document );

  /**
    Serializes the given item into a DOM element and attaches it to the given item.
  */
  AKONADI_XML_EXPORT QDomElement writeItem( const Akonadi::Item& item, QDomElement& parentElem );
}

}

#endif
