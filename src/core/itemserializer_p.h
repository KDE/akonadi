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

#include <memory>

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
    static void deserialize(Item &item, const QByteArray &label, const QByteArray &data, int version, bool external);
    /** throws ItemSerializerException on failure */
    static void deserialize(Item &item, const QByteArray &label, QIODevice &data, int version);
    /** throws ItemSerializerException on failure */
    static void serialize(const Item &item, const QByteArray &label, QByteArray &data, int &version);
    /** throws ItemSerializerException on failure */
    static void serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version);

    /**
     * Throws ItemSerializerException on failure.
     * @param item the item to apply to
     * @param other the item to get values from
     * @since 4.4
     */
    static void apply(Item &item, const Item &other);

    /**
     * Returns a list of parts available in the item payload.
     */
    static QSet<QByteArray> parts(const Item &item);

    /**
     * Returns a list of parts available remotely in the item payload.
     * @param item the item for which to list payload parts
     * @since 4.4
     */
    static QSet<QByteArray> availableParts(const Item &item);

    /**
     * Tries to convert the payload in \a item into type with
     * metatype-id \a metaTypeId.
     * Throws ItemSerializerException or returns an Item w/o payload on failure.
     * @param item the item to convert
     * @param metaTypeID the meta type id used to convert items payload
     * @since 4.6
     */
    static Item convert(const Item &item, int metaTypeId);

    /**
    * Override the plugin-lookup with @p plugin.
    *
    * After calling this each lookup will always return @p plugin.
    * This is useful to inject a special plugin for testing purposes.
    * To reset the plugin, set to 0.
    *
    * @since 4.12
    */
    static void Q_DECL_OVERRIDEPluginLookup(QObject *plugin);
};

/**
  @internal
  Default implementation for serializer plugin.
*/
class DefaultItemSerializerPlugin : public QObject, public ItemSerializerPlugin
{
    Q_OBJECT
    Q_INTERFACES(Akonadi::ItemSerializerPlugin)
public:
    DefaultItemSerializerPlugin();

    bool deserialize(Item &item, const QByteArray &label, QIODevice &data, int version) Q_DECL_OVERRIDE;
    void serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version) Q_DECL_OVERRIDE;
};

/**
 @internal
 Serializer plugin implementation for std::string
*/
class StdStringItemSerializerPlugin : public QObject, public ItemSerializerPlugin
{
    Q_OBJECT
    Q_INTERFACES(Akonadi::ItemSerializerPlugin)
public:
    StdStringItemSerializerPlugin();

    bool deserialize(Item &item, const QByteArray &label, QIODevice &data, int version) Q_DECL_OVERRIDE;
    void serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version) Q_DECL_OVERRIDE;
};

}

#endif
