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

#include "itemserializer_p.h"
#include "item.h"
#include "itemserializerplugin.h"
#include "typepluginloader_p.h"
#include "protocolhelper_p.h"

// Qt
#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QString>

#include <string>

Q_DECLARE_METATYPE(std::string)

namespace Akonadi {

DefaultItemSerializerPlugin::DefaultItemSerializerPlugin()
{
    Item::addToLegacyMapping<QByteArray>(QStringLiteral("application/octet-stream"));
}

bool DefaultItemSerializerPlugin::deserialize(Item &item, const QByteArray &label, QIODevice &data, int)
{
    if (label != Item::FullPayload) {
        return false;
    }

    item.setPayload(data.readAll());
    return true;
}

void DefaultItemSerializerPlugin::serialize(const Item &item, const QByteArray &label, QIODevice &data, int &)
{
    Q_ASSERT(label == Item::FullPayload);
    Q_UNUSED(label);
    data.write(item.payload<QByteArray>());
}

bool StdStringItemSerializerPlugin::deserialize(Item &item, const QByteArray &label, QIODevice &data, int)
{
    if (label != Item::FullPayload) {
        return false;
    }
    std::string str;
    {
        const QByteArray ba = data.readAll();
        str.assign(ba.data(), ba.size());
    }
    item.setPayload(str);
    return true;
}

void StdStringItemSerializerPlugin::serialize(const Item &item, const QByteArray &label, QIODevice &data, int &)
{
    Q_ASSERT(label == Item::FullPayload);
    Q_UNUSED(label);
    const std::string str = item.payload<std::string>();
    data.write(QByteArray::fromRawData(str.data(), str.size()));
}

/*static*/
void ItemSerializer::deserialize(Item &item, const QByteArray &label, const QByteArray &data, int version, bool external)
{
    if (external) {
        const QString fileName = ProtocolHelper::absolutePayloadFilePath(QString::fromUtf8(data));
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            deserialize(item, label, file, version);
            file.close();
        } else {
            qWarning() << "Failed to open external payload:" << fileName << file.errorString();
        }
    } else {
        QBuffer buffer;
        buffer.setData(data);
        buffer.open(QIODevice::ReadOnly);
        buffer.seek(0);
        deserialize(item, label, buffer, version);
        buffer.close();
    }
}

/*static*/
void ItemSerializer::deserialize(Item &item, const QByteArray &label, QIODevice &data, int version)
{
    if (!TypePluginLoader::defaultPluginForMimeType(item.mimeType())->deserialize(item, label, data, version)) {
        qWarning() << "Unable to deserialize payload part:" << label;
        data.seek(0);
        qWarning() << "Payload data was: " << data.readAll();
    }
}

/*static*/
void ItemSerializer::serialize(const Item &item, const QByteArray &label, QByteArray &data, int &version)
{
    QBuffer buffer;
    buffer.setBuffer(&data);
    buffer.open(QIODevice::WriteOnly);
    buffer.seek(0);
    serialize(item, label, buffer, version);
    buffer.close();
}

/*static*/
void ItemSerializer::serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version)
{
    if (!item.hasPayload()) {
        return;
    }
    ItemSerializerPlugin *plugin = TypePluginLoader::pluginForMimeTypeAndClass(item.mimeType(), item.availablePayloadMetaTypeIds());
    plugin->serialize(item, label, data, version);
}

void ItemSerializer::apply(Item &item, const Item &other)
{
    if (!other.hasPayload()) {
        return;
    }

    ItemSerializerPlugin *plugin = TypePluginLoader::pluginForMimeTypeAndClass(item.mimeType(), item.availablePayloadMetaTypeIds());

    ItemSerializerPluginV2 *pluginV2 = dynamic_cast<ItemSerializerPluginV2 *>(plugin);
    if (pluginV2) {
        pluginV2->apply(item, other);
        return;
    }

    // Old-school merge:
    foreach (const QByteArray &part, other.loadedPayloadParts()) {
        QByteArray partData;
        QBuffer buffer;
        buffer.setBuffer(&partData);
        buffer.open(QIODevice::ReadWrite);
        buffer.seek(0);
        int version;
        serialize(other, part, buffer, version);
        buffer.seek(0);
        deserialize(item, part, buffer, version);
    }
}

QSet<QByteArray> ItemSerializer::parts(const Item &item)
{
    if (!item.hasPayload()) {
        return QSet<QByteArray>();
    }
    return TypePluginLoader::pluginForMimeTypeAndClass(item.mimeType(), item.availablePayloadMetaTypeIds())->parts(item);
}

QSet<QByteArray> ItemSerializer::availableParts(const Item &item)
{
    if (!item.hasPayload()) {
        return QSet<QByteArray>();
    }
    ItemSerializerPlugin *plugin = TypePluginLoader::pluginForMimeTypeAndClass(item.mimeType(), item.availablePayloadMetaTypeIds());
    ItemSerializerPluginV2 *pluginV2 = dynamic_cast<ItemSerializerPluginV2 *>(plugin);

    if (pluginV2) {
        return pluginV2->availableParts(item);
    }

    if (item.hasPayload()) {
        return QSet<QByteArray>();
    }

    return QSet<QByteArray>() << Item::FullPayload;
}

Item ItemSerializer::convert(const Item &item, int mtid)
{
//   qDebug() << "asked to convert a" << item.mimeType() << "item to format" << ( mtid ? QMetaType::typeName( mtid ) : "<legacy>" );
    if (!item.hasPayload()) {
        qDebug() << "  -> but item has no payload!";
        return Item();
    }

    if (ItemSerializerPlugin *const plugin = TypePluginLoader::pluginForMimeTypeAndClass(item.mimeType(), QVector<int>(1, mtid), TypePluginLoader::NoDefault)) {
        qDebug() << "  -> found a plugin that feels responsible, trying serialising the payload";
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        int version;
        serialize(item, Item::FullPayload, buffer, version);
        buffer.seek(0);
        qDebug() << "    -> serialized payload into" << buffer.size() << "bytes" << endl
                 << "  -> going to deserialize";
        Item newItem;
        if (plugin->deserialize(newItem, Item::FullPayload, buffer, version)) {
            qDebug() << "    -> conversion successful";
            return newItem;
        } else {
            qDebug() << "    -> conversion FAILED";
        }
    } else {
//     qDebug() << "  -> found NO plugin that feels responsible";
    }
    return Item();
}

void ItemSerializer::overridePluginLookup(QObject *p)
{
    TypePluginLoader::overridePluginLookup(p);
}

}
