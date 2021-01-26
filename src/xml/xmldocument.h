/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_XMLDOCUMENT_H
#define AKONADI_XMLDOCUMENT_H

#include "akonadi-xml_export.h"

#include "collection.h"
#include <item.h>

#include <QDomDocument>

namespace Akonadi
{
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
    explicit XmlDocument(const QString &fileName);
    ~XmlDocument();

    /**
      Parses the given XML file and validates it.
      In case of an error, isValid() will return @c false and
      lastError() will return an error message.

       @see isValid(), lastError()
    */
    bool loadFile(const QString &fileName);

    /**
      Writes the current document into the given file.
    */
    bool writeToFile(const QString &fileName) const;

    /**
      Returns true if the document could be parsed successfully.
      @see lastError()
    */
    Q_REQUIRED_RESULT bool isValid() const;

    /**
      Returns the last error occurred during file loading/parsing.
      Empty if isValid() returns @c true.
      @see isValid()
    */
    Q_REQUIRED_RESULT QString lastError() const;

    /**
      Returns the DOM document for this XML document.
    */
    QDomDocument &document() const;

    /**
      Returns the DOM element representing @p collection.
    */
    Q_REQUIRED_RESULT QDomElement collectionElement(const Collection &collection) const;

    /**
      Returns the DOM element representing the item with the given remote id
    */
    Q_REQUIRED_RESULT QDomElement itemElementByRemoteId(const QString &rid) const;

    /**
     * Returns the DOM element representing the collection with the given remote id
     */
    Q_REQUIRED_RESULT QDomElement collectionElementByRemoteId(const QString &rid) const;

    /**
      Returns the collection with the given remote id.
    */
    Q_REQUIRED_RESULT Collection collectionByRemoteId(const QString &rid) const;

    /**
      Returns the item with the given remote id.
    */
    Q_REQUIRED_RESULT Item itemByRemoteId(const QString &rid, bool includePayload = true) const;

    /**
      Returns the collections defined in this document.
    */
    Q_REQUIRED_RESULT Collection::List collections() const;

    /**
      Returns the tags defined in this document.
    */
    Q_REQUIRED_RESULT Tag::List tags() const;

    /**
      Returns the immediate child collections of @p parentCollection.
    */
    Q_REQUIRED_RESULT Collection::List childCollections(const Collection &parentCollection) const;

    /**
      Returns the items in the given collection.
    */
    Q_REQUIRED_RESULT Item::List items(const Collection &collection, bool includePayload = true) const;

private:
    Q_DISABLE_COPY(XmlDocument)
    XmlDocumentPrivate *const d;
};

}

#endif
