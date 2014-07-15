/*
    Copyright (c) 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_XMLREADER_H
#define AKONADI_XMLREADER_H

#include "akonadi-xml_export.h"

#include "collection.h"
#include "item.h"

#include <QtXml/QDomElement>

namespace Akonadi {

class Attribute;

/**
  Low-level methods to transform DOM elements into the corresponding Akonadi objects.
  @see Akonadi::XmlDocument
*/
namespace XmlReader
{
  /**
    Converts an attribute element.
  */
  AKONADI_XML_EXPORT Attribute* elementToAttribute( const QDomElement &elem );

  /**
    Reads all attributes that are immediate children of @p elem and adds them
    to @p entity.
  */
  AKONADI_XML_EXPORT void readAttributes( const QDomElement &elem, Entity &entity );

  /**
    Converts a collection element.
  */
  AKONADI_XML_EXPORT Collection elementToCollection( const QDomElement &elem );

  /**
    Reads recursively all collections starting from the given DOM element.
  */
  AKONADI_XML_EXPORT Collection::List readCollections( const QDomElement &elem );

  /**
    Converts a tag element.
  */
  AKONADI_XML_EXPORT Tag elementToTag( const QDomElement &elem );

  /**
    Reads recursively all tags starting from the given DOM element.
  */
  AKONADI_XML_EXPORT Tag::List readTags( const QDomElement &elem );

  /**
    Converts an item element.
  */
  AKONADI_XML_EXPORT Item elementToItem( const QDomElement &elem, bool includePayload = true );
}

}

#endif
