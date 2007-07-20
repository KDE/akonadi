/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMSERIALIZERPLUGIN_H
#define AKONADI_ITEMSERIALIZERPLUGIN_H

#include "libakonadi_export.h"

class QString;
class QStringList;
class QIODevice;

namespace Akonadi {

class Item;

/**
  Base class for PIM item type serializer plugins.
  Serializer plugins convert between the payload of Akonadi::Item objects and
  a textual or binary representation of the actual content data.
  This allows to easily add support for new types to Akonadi.

  <h4>How to write an Akonadi serializer plugin</h4>

  The following describes the basic steps needed to write an Akonadi
  serializer plugin.

  <h5>(de-)Serialization Code</h5>

  Create a class that inherits Akonadi::ItemSerializerPlugin and
  implement its two pure virtual methods, serialize() and deserialize().

  <h5>Plugin Framework</h5>

  @todo Factory macro

   @todo Desktop file example
*/
class AKONADI_EXPORT ItemSerializerPlugin
{
public:
    /**
      Destructor
    */
    virtual ~ItemSerializerPlugin();

    /**
      Convert serialized item data provided in @p data into payload for @p item.
      @param item The item to which the payload should be added.
      It is guaranteed to have a mimetype matching one of the supported mimetypes of this
      plugin. However it might contain a unsuited payload added manually by the application
      developer. Verifying the payload type in case a payload is already available is recommended
      therefore.
      @param label The part identifier of the part to deserialize. @p label will
      be one of the item parts returned by parts().
      @param data An QIODevice providing access to the serialized data. The QIODevice is opened in
      read-only mode and positioned at the beginning. The QIODevice is guaranteed to be valid.
    */
    virtual void deserialize( Item& item, const QString& label, QIODevice& data ) = 0;

    /**
      Convert the payload object provided in @p item into its serialzed form into @p data.
      @param item The item which contains the payload.
      It is guaranteed to have a mimetype matching one of the supported mimetypes of this
      plugin as well as the existence of a payload object. However it might contain an unsupported
      payload added manually by the application developer. Verifying the payload type is recommended
      therefore.
      @param label The part identifier of the part to serialize. @p label will
      be one of the item parts returned by parts().
      @param data The QIODevice where the serialized data should be writtne to.
      The QIODevice is opened in write-only mode and positioned at the beginning.
      The QIODevice is guaranteed to be valid.
    */
    virtual void serialize( const Item& item, const QString& label, QIODevice& data ) = 0;

    /**
      Returns a list of available parts for the given item payload.
      The default implementation returns Item::PartBody.
      @param item The item.
    */
    virtual QStringList parts( const Item &item ) const;
};


class AKONADI_EXPORT ItemSerializerException
{
};

}

#endif
