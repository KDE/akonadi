/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemserializer_p.h"
#include "item.h"
#include "itemserializerplugin.h"
#include "typepluginloader_p.h"
#include "protocolhelper_p.h"

#include "private/externalpartstorage_p.h"
#include "private/compressionstream_p.h"

#include "akonadicore_debug.h"

// Qt
#include <QBuffer>
#include <QFile>
#include <QIODevice>
#include <QString>

#include <string>

Q_DECLARE_METATYPE(std::string)

namespace Akonadi
{

DefaultItemSerializerPlugin::DefaultItemSerializerPlugin() = default;

bool DefaultItemSerializerPlugin::deserialize(Item &item, const QByteArray &label, QIODevice &data, int /*version*/)
{
    if (label != Item::FullPayload) {
        return false;
    }

    item.setPayload(data.readAll());
    return true;
}

void DefaultItemSerializerPlugin::serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version)
{
    Q_UNUSED(version)
    Q_ASSERT(label == Item::FullPayload);
    Q_UNUSED(label);
    data.write(item.payload<QByteArray>());
}

bool StdStringItemSerializerPlugin::deserialize(Item &item, const QByteArray &label, QIODevice &data, int /*version*/)
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

void StdStringItemSerializerPlugin::serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version)
{
    Q_UNUSED(version)
    Q_ASSERT(label == Item::FullPayload);
    Q_UNUSED(label);
    const std::string str = item.payload<std::string>();
    data.write(QByteArray::fromRawData(str.data(), str.size()));
}

/*static*/
void ItemSerializer::deserialize(Item &item, const QByteArray &label, const QByteArray &data,
                                 int version, PayloadStorage storage)
{
    if (storage == Internal) {
        QBuffer buffer;
        buffer.setData(data);
        buffer.open(QIODevice::ReadOnly);
        deserialize(item, label, buffer, version);
        buffer.close();
    } else {
        QFile file;
        if (storage == External) {
            file.setFileName(ExternalPartStorage::resolveAbsolutePath(data));
        } else if (storage == Foreign) {
            file.setFileName(QString::fromUtf8(data));
        }

        if (file.open(QIODevice::ReadOnly)) {
            deserialize(item, label, file, version);
            file.close();
        } else {
            qCWarning(AKONADICORE_LOG) << "Failed to open"
                    << ((storage == External) ? "external" : "foreign") << "payload:"
                    << file.fileName() << file.errorString();
        }
    }
}

/*static*/
void ItemSerializer::deserialize(Item &item, const QByteArray &label, QIODevice &data, int version)
{
    auto *plugin = TypePluginLoader::defaultPluginForMimeType(item.mimeType());

    const auto handleError = [&](QIODevice &device, bool compressed) {
        device.seek(0);
        QByteArray data;
        if (compressed) {
            CompressionStream decompressor(&device);
            decompressor.open(QIODevice::ReadOnly);
            data = decompressor.readAll();
        } else {
            data = device.readAll();
        }

        qCWarning(AKONADICORE_LOG) << "Unable to deserialize payload part:" << label << "in item" << item.id() << "collection" << item.parentCollection().id();
        qCWarning(AKONADICORE_LOG) << (compressed ? "Decompressed" : "") << "payload data was: " << data;
    };

    if (CompressionStream::isCompressed(&data)) {
        CompressionStream decompressor(&data);
        decompressor.open(QIODevice::ReadOnly);
        if (!plugin->deserialize(item, label, decompressor, version)) {
            handleError(decompressor, true);
        }
        if (decompressor.error()) {
            qCWarning(AKONADICORE_LOG) << "Deserialization failed due to decompression error:" << QString::fromStdString(decompressor.error().message());
        }
    } else {
        if (!plugin->deserialize(item, label, data, version)) {
            handleError(data, false);
        }
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

    CompressionStream compressor(&data);
    compressor.open(QIODevice::WriteOnly);
    plugin->serialize(item, label, compressor, version);
}

void ItemSerializer::apply(Item &item, const Item &other)
{
    if (!other.hasPayload()) {
        return;
    }

    ItemSerializerPlugin *plugin = TypePluginLoader::pluginForMimeTypeAndClass(item.mimeType(), item.availablePayloadMetaTypeIds());
    plugin->apply(item, other);
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
    return plugin->availableParts(item);
}

QSet<QByteArray> ItemSerializer::allowedForeignParts(const Item &item)
{
    if (!item.hasPayload()) {
        return QSet<QByteArray>();
    }

    ItemSerializerPlugin *plugin = TypePluginLoader::pluginForMimeTypeAndClass(item.mimeType(), item.availablePayloadMetaTypeIds());
    return plugin->allowedForeignParts(item);
}

Item ItemSerializer::convert(const Item &item, int mtid)
{
    qCDebug(AKONADICORE_LOG) << "asked to convert a" << item.mimeType() << "item to format" << ( mtid ? QMetaType::typeName( mtid ) : "<legacy>" );
    if (!item.hasPayload()) {
        qCDebug(AKONADICORE_LOG) << "  -> but item has no payload!";
        return Item();
    }

    if (ItemSerializerPlugin *const plugin = TypePluginLoader::pluginForMimeTypeAndClass(item.mimeType(), QVector<int>(1, mtid), TypePluginLoader::NoDefault)) {
        qCDebug(AKONADICORE_LOG) << "  -> found a plugin that feels responsible, trying serialising the payload";
        QBuffer buffer;
        buffer.open(QIODevice::ReadWrite);
        int version = 0;
        serialize(item, Item::FullPayload, buffer, version);
        buffer.seek(0);
        qCDebug(AKONADICORE_LOG) << "    -> serialized payload into" << buffer.size() << "bytes\n"
                                 << "  -> going to deserialize";
        Item newItem;
        if (plugin->deserialize(newItem, Item::FullPayload, buffer, version)) {
            qCDebug(AKONADICORE_LOG) << "    -> conversion successful";
            return newItem;
        } else {
            qCDebug(AKONADICORE_LOG) << "    -> conversion FAILED";
        }
    } else {
//     qCDebug(AKONADICORE_LOG) << "  -> found NO plugin that feels responsible";
    }
    return Item();
}

void ItemSerializer::overridePluginLookup(QObject *p)
{
    TypePluginLoader::overridePluginLookup(p);
}

} // namespace Akonadi

