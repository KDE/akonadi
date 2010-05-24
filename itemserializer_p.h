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

#ifndef AKONADI_ITEM_SERIALIZER_P_H
#define AKONADI_ITEM_SERIALIZER_P_H

#include <QtCore/QByteArray>
#include <QtCore/QSet>

#include "akonadiprivate_export.h"

#include "itemserializerplugin.h"

class QIODevice;

namespace Akonadi {

class Item;

/**
  @internal
  Serialization/Deserialization of item parts, serializer plugin management.
*/
class AKONADI_TESTS_EXPORT ItemSerializer
{
  public:
      /** throws ItemSerializerException on failure */
      static void deserialize( Item& item, const QByteArray& label, const QByteArray& data, int version, bool external );
      /** throws ItemSerializerException on failure */
      static void deserialize( Item& item, const QByteArray& label, QIODevice& data, int version );
      /** throws ItemSerializerException on failure */
      static void serialize( const Item& item, const QByteArray& label, QByteArray& data, int &version );
      /** throws ItemSerializerException on failure */
      static void serialize( const Item& item, const QByteArray& label, QIODevice& data, int &version );

      /**
       * Throws ItemSerializerException on failure.
       *
       * @since 4.4
       */
      static void apply( Item& item, const Item &other );

      /**
       * Returns a list of parts available in the item payload.
       */
      static QSet<QByteArray> parts( const Item &item );

      /**
       * Returns a list of parts available remotely in the item payload.
       *
       * @since 4.4
       */
      static QSet<QByteArray> availableParts( const Item &item );
};

/**
  @internal
  Default implementation for serializer plugin.
*/
class DefaultItemSerializerPlugin : public ItemSerializerPlugin
{
  public:
    DefaultItemSerializerPlugin();

    bool deserialize( Item&, const QByteArray&, QIODevice&, int );
    void serialize( const Item&, const QByteArray&, QIODevice&, int& );
};

}

#endif
