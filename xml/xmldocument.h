/*
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

#ifndef AKONADI_XMLDOCUMENT_H
#define AKONADI_XMLDOCUMENT_H

#include "akonadi-xml_export.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QtXml/QDomDocument>

namespace Akonadi {

class XmlDocumentPrivate;

/**
  Represents a document of the KNUT XML serialization format for Akonadi objects.
*/
class AKONADI_XML_EXPORT XmlDocument
{
  public:
    /**
      Creates an empty document.
    */
    XmlDocument();

    /**
      Creates a new XmlDocument object and calls loadFile().

      @see loadFile()
    */
    XmlDocument( const QString &fileName );
    ~XmlDocument();

    /**
      Parses the given XML file and validates it.
      In case of an error, isValid() will return @c false and
      lastError() will return an error message.

       @see isValid(), lastError()
    */
    bool loadFile( const QString &fileName );

    /**
      Writes the current document into the given file.
    */
    bool writeToFile( const QString &fileName ) const;

    /**
      Returns true if the document could be parsed successfully.
      @see lastError()
    */
    bool isValid() const;

    /**
      Returns the last error occurred during file loading/parsing.
      Empty if isValid() returns @c true.
      @see isValid()
    */
    QString lastError() const;

    /**
      Returns the DOM document for this XML document.
    */
    QDomDocument& document() const;

    /**
      Returns the DOM element representing the collection with the given remote id
      @deprecated Not HRID aware, use collectionElement() instead
    */
    KDE_DEPRECATED QDomElement collectionElementByRemoteId( const QString &rid ) const;

    /**
      Returns the DOM element representing @p collection.
    */
    QDomElement collectionElement( const Collection &collection ) const;

    /**
      Returns the DOM element representing the item with the given remote id
    */
    QDomElement itemElementByRemoteId( const QString &rid ) const;

    /**
      Returns the collection with the given remote id.
    */
    Collection collectionByRemoteId( const QString &rid ) const;

    /**
      Returns the item with the given remote id.
    */
    Item itemByRemoteId( const QString& rid, bool includePayload = true ) const;

    /**
      Returns the collections defined in this document.
    */
    Collection::List collections() const;

    /**
      Returns the tags defined in this document.
    */
    Tag::List tags() const;

    /**
      Returns immediate child collections of the specified parent collection.
      @deprecated Not HRID aware, use childCollections( Akonadi::Collection ) instead
    */
    KDE_DEPRECATED Collection::List childCollections( const QString &parentCollectionRid ) const;

    /**
      Returns the immediate child collections of @p parentCollection.
    */
    Collection::List childCollections( const Collection &parentCollection ) const;

    /**
      Returns the items in the given collection.
    */
    Item::List items( const Collection& collection, bool includePayload = true ) const;

  private:
    XmlDocumentPrivate * const d;
};

}

#endif
