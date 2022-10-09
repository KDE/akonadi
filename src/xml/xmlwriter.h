/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadi-xml_export.h"

#include <QDomElement>

namespace Akonadi
{
class Attribute;
class Collection;
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
Q_REQUIRED_RESULT AKONADI_XML_EXPORT QDomElement attributeToElement(Attribute *attr, QDomDocument &document);

/**
  Serializes all attributes of the given Akonadi object into the given parent element.
*/
AKONADI_XML_EXPORT void writeAttributes(const Item &entity, QDomElement &parentElem);

/**
  Serializes all attributes of the given Akonadi object into the given parent element.
*/
AKONADI_XML_EXPORT void writeAttributes(const Collection &entity, QDomElement &parentElem);

/**
  Creates a collection element for the given document, not yet attached to the DOM tree.
*/
Q_REQUIRED_RESULT AKONADI_XML_EXPORT QDomElement collectionToElement(const Collection &collection, QDomDocument &document);

/**
  Serializes the given collection into a DOM element with the given parent.
*/
AKONADI_XML_EXPORT QDomElement writeCollection(const Collection &collection, QDomElement &parentElem);

/**
  Creates an item element for the given document, not yet attached to the DOM tree
*/
Q_REQUIRED_RESULT AKONADI_XML_EXPORT QDomElement itemToElement(const Item &item, QDomDocument &document);

/**
  Serializes the given item into a DOM element and attaches it to the given item.
*/
AKONADI_XML_EXPORT QDomElement writeItem(const Item &item, QDomElement &parentElem);
}

}
