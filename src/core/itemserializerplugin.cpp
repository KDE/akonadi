/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemserializerplugin.h"
#include "item.h"
#include "itemserializer_p.h"

#include <QBuffer>

using namespace Akonadi;

ItemSerializerPlugin::~ItemSerializerPlugin() = default;

QSet<QByteArray> ItemSerializerPlugin::parts(const Item &item) const
{
    if (!item.hasPayload()) {
        return {};
    }

    return { Item::FullPayload };
}

void ItemSerializerPlugin::overridePluginLookup(QObject *p)
{
    ItemSerializer::overridePluginLookup(p);
}

QSet<QByteArray> ItemSerializerPlugin::availableParts(const Item &item) const
{
    if (!item.hasPayload()) {
        return {};
    }

    return { Item::FullPayload };
}

void ItemSerializerPlugin::apply(Item &item, const Item &other)
{
    Q_FOREACH (const QByteArray &part, other.loadedPayloadParts()) {
        QByteArray partData;
        QBuffer buffer;
        buffer.setBuffer(&partData);
        buffer.open(QIODevice::ReadWrite);
        buffer.seek(0);
        int version;
        // NOTE: we can't just pass other.payloadData() into deserialize(),
        // because that does not preserve payload version.
        serialize(other, part, buffer, version);
        buffer.seek(0);
        deserialize(item, part, buffer, version);
    }
}

QSet<QByteArray> ItemSerializerPlugin::allowedForeignParts(const Item &item) const
{
    if (!item.hasPayload()) {
        return {};
    }

    return { Item::FullPayload };
}
