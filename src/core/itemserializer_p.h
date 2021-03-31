/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QSet>

#include "akonaditests_export.h"

#include "itemserializerplugin.h"

#include <memory>

class QIODevice;

namespace Akonadi
{
class Item;

/**
  @internal
  Serialization/Deserialization of item parts, serializer plugin management.
*/
class AKONADI_TESTS_EXPORT ItemSerializer
{
public:
    enum PayloadStorage {
        Internal,
        External,
        Foreign,
    };

    /** throws ItemSerializerException on failure */
    static void deserialize(Item &item, const QByteArray &label, const QByteArray &data, int version, PayloadStorage storage);
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
     * Returns list of parts of the item payload that can be stored using
     * foreign payload.
     *
     * @since 5.7
     */
    static QSet<QByteArray> allowedForeignParts(const Item &item);

    /**
     * Tries to convert the payload in \a item into type with
     * metatype-id \a metaTypeId.
     * Throws ItemSerializerException or returns an Item w/o payload on failure.
     * @param item the item to convert
     * @param metaTypeId the meta type id used to convert items payload
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
    static void overridePluginLookup(QObject *plugin);
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

    bool deserialize(Item &item, const QByteArray &label, QIODevice &data, int version) override;
    void serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version) override;
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

    bool deserialize(Item &item, const QByteArray &label, QIODevice &data, int version) override;
    void serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version) override;
};

}

