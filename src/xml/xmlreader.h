/*
    SPDX-FileCopyrightText: 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadi-xml_export.h"

#include "collection.h"
#include "item.h"

#include <QDomElement>

namespace Akonadi
{
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
AKONADI_XML_EXPORT Attribute *elementToAttribute(const QDomElement &elem);

/**
  Reads all attributes that are immediate children of @p elem and adds them
  to @p item.
*/
AKONADI_XML_EXPORT void readAttributes(const QDomElement &elem, Item &item);

/**
  Reads all attributes that are immediate children of @p elem and adds them
  to @p collection.
*/
AKONADI_XML_EXPORT void readAttributes(const QDomElement &elem, Collection &collection);

/**
  Converts a collection element.
*/
Q_REQUIRED_RESULT AKONADI_XML_EXPORT Collection elementToCollection(const QDomElement &elem);

/**
  Reads recursively all collections starting from the given DOM element.
*/
Q_REQUIRED_RESULT AKONADI_XML_EXPORT Collection::List readCollections(const QDomElement &elem);

/**
  Converts a tag element.
*/
Q_REQUIRED_RESULT AKONADI_XML_EXPORT Tag elementToTag(const QDomElement &elem);

/**
  Reads recursively all tags starting from the given DOM element.
*/
Q_REQUIRED_RESULT AKONADI_XML_EXPORT Tag::List readTags(const QDomElement &elem);

/**
  Converts an item element.
*/
Q_REQUIRED_RESULT AKONADI_XML_EXPORT Item elementToItem(const QDomElement &elem, bool includePayload = true);
}

}

