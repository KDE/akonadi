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

#include "itemserializerplugin.h"
#include "item.h"
#include "itemserializer_p.h"

#include <QBuffer>

using namespace Akonadi;

ItemSerializerPlugin::~ItemSerializerPlugin()
{
}

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
