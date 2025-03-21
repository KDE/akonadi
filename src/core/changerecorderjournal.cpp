/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akonadicore_debug.h"
#include "changerecorderjournal_p.h"
#include "protocol_p.h"

#include <QDataStream>
#include <QFile>
#include <QQueue>
#include <QSettings>

using namespace Akonadi;

namespace
{
constexpr quint64 s_currentVersion = Q_UINT64_C(0x000A00000000);
constexpr quint64 s_versionMask = Q_UINT64_C(0xFFFF00000000);
constexpr quint64 s_sizeMask = Q_UINT64_C(0x0000FFFFFFFF);
}

Protocol::ChangeNotificationPtr ChangeRecorderJournalReader::loadQSettingsNotification(QSettings *settings)
{
    switch (static_cast<LegacyType>(settings->value(QStringLiteral("type")).toInt())) {
    case Item:
        return loadQSettingsItemNotification(settings);
    case Collection:
        return loadQSettingsCollectionNotification(settings);
    case Tag:
    case Relation:
    case InvalidType:
    default:
        qWarning() << "Unexpected notification type in legacy store";
        return {};
    }
}

QQueue<Protocol::ChangeNotificationPtr> ChangeRecorderJournalReader::loadFrom(QFile *device, bool &needsFullSave)
{
    QDataStream stream(device);
    stream.setVersion(QDataStream::Qt_4_6);

    QByteArray sessionId;
    int type;

    QQueue<Protocol::ChangeNotificationPtr> list;

    quint64 sizeAndVersion;
    stream >> sizeAndVersion;

    const quint64 size = sizeAndVersion & s_sizeMask;
    const quint64 version = (sizeAndVersion & s_versionMask) >> 32;

    quint64 startOffset = 0;
    if (version >= 1) {
        stream >> startOffset;
    }

    // If we skip the first N items, then we'll need to rewrite the file on saving.
    // Also, if the file is old, it needs to be rewritten.
    needsFullSave = startOffset > 0 || version == 0;

    for (quint64 i = 0; i < size && !stream.atEnd(); ++i) {
        Protocol::ChangeNotificationPtr msg;
        stream >> sessionId;
        stream >> type;

        if (stream.status() != QDataStream::Ok) {
            qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting. Corrupt file:" << device->fileName();
            break;
        }

        switch (static_cast<LegacyType>(type)) {
        case Item:
            msg = loadItemNotification(stream, version);
            break;
        case Collection:
            msg = loadCollectionNotification(stream, version);
            break;
        case Tag:
            msg = loadTagNotification(stream, version);
            break;
        case Relation:
            // Just load it but discard the result, we don't support relations anymore
            loadRelationNotification(stream, version);
            break;
        default:
            qCWarning(AKONADICORE_LOG) << "Unknown notification type";
            break;
        }

        if (i < startOffset) {
            continue;
        }

        if (msg && msg->isValid()) {
            msg->setSessionId(sessionId);
            list << msg;
        }
    }

    return list;
}

void ChangeRecorderJournalWriter::saveTo(const QQueue<Protocol::ChangeNotificationPtr> &notifications, QIODevice *device)
{
    // Version 0 of this file format was writing a quint64 count, followed by the notifications.
    // Version 1 bundles a version number into that quint64, to be able to detect a version number at load time.

    const quint64 countAndVersion = static_cast<quint64>(notifications.count()) | s_currentVersion;

    QDataStream stream(device);
    stream.setVersion(QDataStream::Qt_4_6);

    stream << countAndVersion;
    stream << quint64(0); // no start offset

    // qCDebug(AKONADICORE_LOG) << "Saving" << pendingNotifications.count() << "notifications (full save)";

    for (int i = 0; i < notifications.count(); ++i) {
        const Protocol::ChangeNotificationPtr &msg = notifications.at(i);

        // We deliberately don't use Factory::serialize(), because the internal
        // serialization format could change at any point

        stream << msg->sessionId();
        stream << int(mapToLegacyType(msg->type()));
        switch (msg->type()) {
        case Protocol::Command::ItemChangeNotification:
            saveItemNotification(stream, Protocol::cmdCast<Protocol::ItemChangeNotification>(msg));
            break;
        case Protocol::Command::CollectionChangeNotification:
            saveCollectionNotification(stream, Protocol::cmdCast<Protocol::CollectionChangeNotification>(msg));
            break;
        case Protocol::Command::TagChangeNotification:
            saveTagNotification(stream, Protocol::cmdCast<Protocol::TagChangeNotification>(msg));
            break;
        default:
            qCWarning(AKONADICORE_LOG) << "Unexpected type?";
            return;
        }
    }
}

Protocol::ChangeNotificationPtr ChangeRecorderJournalReader::loadQSettingsItemNotification(QSettings *settings)
{
    auto msg = Protocol::ItemChangeNotificationPtr::create();
    msg->setSessionId(settings->value(QStringLiteral("sessionId")).toByteArray());
    msg->setOperation(mapItemOperation(static_cast<LegacyOp>(settings->value(QStringLiteral("op")).toInt())));
    Protocol::FetchItemsResponse item;
    item.setId(settings->value(QStringLiteral("uid")).toLongLong());
    item.setRemoteId(settings->value(QStringLiteral("rid")).toString());
    item.setMimeType(settings->value(QStringLiteral("mimeType")).toString());
    msg->setItems({std::move(item)});
    msg->addMetadata("FETCH_ITEM");
    msg->setResource(settings->value(QStringLiteral("resource")).toByteArray());
    msg->setParentCollection(settings->value(QStringLiteral("parentCol")).toLongLong());
    msg->setParentDestCollection(settings->value(QStringLiteral("parentDestCol")).toLongLong());
    const QStringList list = settings->value(QStringLiteral("itemParts")).toStringList();
    QSet<QByteArray> itemParts;
    for (const QString &entry : list) {
        itemParts.insert(entry.toLatin1());
    }
    msg->setItemParts(itemParts);
    return msg;
}

Protocol::ChangeNotificationPtr ChangeRecorderJournalReader::loadQSettingsCollectionNotification(QSettings *settings)
{
    auto msg = Protocol::CollectionChangeNotificationPtr::create();
    msg->setSessionId(settings->value(QStringLiteral("sessionId")).toByteArray());
    msg->setOperation(mapCollectionOperation(static_cast<LegacyOp>(settings->value(QStringLiteral("op")).toInt())));
    Protocol::FetchCollectionsResponse collection;
    collection.setId(settings->value(QStringLiteral("uid")).toLongLong());
    collection.setRemoteId(settings->value(QStringLiteral("rid")).toString());
    msg->setCollection(std::move(collection));
    msg->addMetadata("FETCH_COLLECTION");
    msg->setResource(settings->value(QStringLiteral("resource")).toByteArray());
    msg->setParentCollection(settings->value(QStringLiteral("parentCol")).toLongLong());
    msg->setParentDestCollection(settings->value(QStringLiteral("parentDestCol")).toLongLong());
    const QStringList list = settings->value(QStringLiteral("itemParts")).toStringList();
    QSet<QByteArray> changedParts;
    for (const QString &entry : list) {
        changedParts.insert(entry.toLatin1());
    }
    msg->setChangedParts(changedParts);
    return msg;
}

QList<Protocol::FetchTagsResponse> loadTags(QDataStream &stream)
{
    QList<Protocol::FetchTagsResponse> tags;
    int cnt = 0;
    stream >> cnt;
    tags.reserve(cnt);
    for (int i = 0; i < cnt; ++i) {
        qint64 id;
        qint64 parentId;
        QByteArray gid;
        QByteArray type;
        QByteArray remoteId;
        QMap<QByteArray, QByteArray> attributes;
        stream >> id >> parentId >> gid >> type >> remoteId >> attributes;
        Protocol::FetchTagsResponse tag;
        tag.setId(id);
        tag.setParentId(parentId);
        tag.setGid(gid);
        tag.setType(type);
        tag.setRemoteId(remoteId);
        tag.setAttributes(attributes);
        tags.emplace_back(std::move(tag));
    }
    return tags;
}

Protocol::ChangeNotificationPtr ChangeRecorderJournalReader::loadItemNotification(QDataStream &stream, quint64 version)
{
    QByteArray resource;
    QByteArray destinationResource;
    int operation;
    int entityCnt;
    qint64 uid;
    qint64 parentCollection;
    qint64 parentDestCollection;
    QString remoteId;
    QString mimeType;
    QString remoteRevision;
    QSet<QByteArray> itemParts;
    QSet<QByteArray> addedFlags;
    QSet<QByteArray> removedFlags;
    QList<Protocol::FetchTagsResponse> addedTags;
    QList<Protocol::FetchTagsResponse> removedTags;
    QList<Protocol::FetchItemsResponse> items;

    auto msg = Protocol::ItemChangeNotificationPtr::create();

    if (version == 1) {
        stream >> operation;
        stream >> uid;
        stream >> remoteId;
        stream >> resource;
        stream >> parentCollection;
        stream >> parentDestCollection;
        stream >> mimeType;
        stream >> itemParts;

        Protocol::FetchItemsResponse item;
        item.setId(uid);
        item.setRemoteId(remoteId);
        item.setMimeType(mimeType);
        items.push_back(std::move(item));
        msg->addMetadata("FETCH_ITEM");
    } else if (version >= 2) {
        stream >> operation;
        stream >> entityCnt;
        if (version >= 7) {
            QByteArray ba;
            qint64 i64;
            int i;
            QDateTime dt;
            QString str;
            QList<QByteArray> bav;
            QList<qint64> i64v;
            QMap<QByteArray, QByteArray> babaMap;
            int cnt;
            for (int j = 0; j < entityCnt; ++j) {
                Protocol::FetchItemsResponse item;
                stream >> i64;
                item.setId(i64);
                stream >> i;
                item.setRevision(i);
                stream >> i64;
                item.setParentId(i64);
                stream >> str;
                item.setRemoteId(str);
                stream >> str;
                item.setRemoteRevision(str);
                stream >> str;
                item.setGid(str);
                stream >> i64;
                item.setSize(i64);
                stream >> str;
                item.setMimeType(str);
                stream >> dt;
                item.setMTime(dt);
                stream >> bav;
                item.setFlags(bav);
                item.setTags(loadTags(stream));
                stream >> i64v;
                item.setVirtualReferences(i64v);
                stream >> cnt;
                for (int k = 0; k < cnt; ++k) {
                    stream >> i64; // left
                    stream >> ba; // left mimetype
                    stream >> i64; // right
                    stream >> ba; // right mimetype
                    stream >> ba; // type
                    stream >> ba; // remoteid
                }
                stream >> cnt;
                QList<Protocol::Ancestor> ancestors;
                for (int k = 0; k < cnt; ++k) {
                    Protocol::Ancestor ancestor;
                    stream >> i64;
                    ancestor.setId(i64);
                    stream >> str;
                    ancestor.setRemoteId(str);
                    stream >> str;
                    ancestor.setName(str);
                    stream >> babaMap;
                    ancestor.setAttributes(babaMap);
                    ancestors << ancestor;
                }
                item.setAncestors(ancestors);
                stream >> cnt;
                QList<Protocol::StreamPayloadResponse> parts;
                for (int k = 0; k < cnt; ++k) {
                    Protocol::StreamPayloadResponse part;
                    stream >> ba;
                    part.setPayloadName(ba);
                    Protocol::PartMetaData metaData;
                    stream >> ba;
                    metaData.setName(ba);
                    stream >> i64;
                    metaData.setSize(i64);
                    stream >> i;
                    metaData.setVersion(i);
                    stream >> i;
                    metaData.setStorageType(static_cast<Protocol::PartMetaData::StorageType>(i));
                    part.setMetaData(metaData);
                    stream >> ba;
                    part.setData(ba);
                    parts << part;
                }
                item.setParts(parts);
                stream >> bav;
                item.setCachedParts(bav);
                items.push_back(std::move(item));
            }
        } else {
            for (int j = 0; j < entityCnt; ++j) {
                stream >> uid;
                stream >> remoteId;
                stream >> remoteRevision;
                stream >> mimeType;
                if (stream.status() != QDataStream::Ok) {
                    qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting";
                    return msg;
                }
                Protocol::FetchItemsResponse item;
                item.setId(uid);
                item.setRemoteId(remoteId);
                item.setRemoteRevision(remoteRevision);
                item.setMimeType(mimeType);
                items.push_back(std::move(item));
            }
            msg->addMetadata("FETCH_ITEM");
        }
        stream >> resource;
        stream >> destinationResource;
        stream >> parentCollection;
        stream >> parentDestCollection;
        stream >> itemParts;
        stream >> addedFlags;
        stream >> removedFlags;
        if (version >= 0xA) {
            addedTags = loadTags(stream);
            removedTags = loadTags(stream);
        } else if (version >= 3) {
            QSet<qint64> tagIds;
            stream >> tagIds;
            for (const auto &tagId : tagIds) {
                addedTags.emplace_back(tagId);
            }
            tagIds.clear();
            stream >> tagIds;
            for (const auto &tagId : tagIds) {
                removedTags.emplace_back(tagId);
            }
        }
        if (version >= 8) {
            bool boolean;
            stream >> boolean;
            msg->setMustRetrieve(boolean);
        }
    } else {
        qCWarning(AKONADICORE_LOG) << "Error version is not correct here" << version;
        return msg;
    }
    if (version >= 5) {
        msg->setOperation(static_cast<Protocol::ItemChangeNotification::Operation>(operation));
    } else {
        msg->setOperation(mapItemOperation(static_cast<LegacyOp>(operation)));
    }
    msg->setItems(items);
    msg->setResource(resource);
    msg->setDestinationResource(destinationResource);
    msg->setParentCollection(parentCollection);
    msg->setParentDestCollection(parentDestCollection);
    msg->setItemParts(itemParts);
    msg->setAddedFlags(addedFlags);
    msg->setRemovedFlags(removedFlags);
    msg->setAddedTags(addedTags);
    msg->setRemovedTags(removedTags);
    return msg;
}

void saveTags(QDataStream &stream, const QList<Protocol::FetchTagsResponse> &tags)
{
    stream << static_cast<int>(tags.count());
    for (const auto &tag : tags) {
        stream << tag.id() << tag.parentId() << tag.gid() << tag.type() << tag.remoteId() << tag.attributes();
    }
}

void ChangeRecorderJournalWriter::saveItemNotification(QDataStream &stream, const Protocol::ItemChangeNotification &msg)
{
    // Version 10

    stream << int(msg.operation());
    const auto &items = msg.items();
    stream << static_cast<int>(items.count());
    for (const auto &item : items) {
        stream << item.id() << item.revision() << item.parentId() << item.remoteId() << item.remoteRevision() << item.gid() << item.size() << item.mimeType()
               << item.mTime() << item.flags();
        saveTags(stream, item.tags());
        stream << item.virtualReferences();
        const auto ancestors = item.ancestors();
        stream << static_cast<int>(ancestors.count());
        for (const auto &ancestor : ancestors) {
            stream << ancestor.id() << ancestor.remoteId() << ancestor.name() << ancestor.attributes();
        }
        const auto parts = item.parts();
        stream << static_cast<int>(parts.count());
        for (const auto &part : parts) {
            const auto metaData = part.metaData();
            stream << part.payloadName() << metaData.name() << metaData.size() << metaData.version() << static_cast<int>(metaData.storageType()) << part.data();
        }
        stream << item.cachedParts();
    }
    stream << msg.resource();
    stream << msg.destinationResource();
    stream << quint64(msg.parentCollection());
    stream << quint64(msg.parentDestCollection());
    stream << msg.itemParts();
    stream << msg.addedFlags();
    stream << msg.removedFlags();
    saveTags(stream, msg.addedTags());
    saveTags(stream, msg.removedTags());
    stream << msg.mustRetrieve();
}

Protocol::ChangeNotificationPtr ChangeRecorderJournalReader::loadCollectionNotification(QDataStream &stream, quint64 version)
{
    QByteArray resource;
    QByteArray destinationResource;
    int operation;
    int entityCnt;
    quint64 uid;
    quint64 parentCollection;
    quint64 parentDestCollection;
    QString remoteId;
    QString remoteRevision;
    QString dummyString;
    QSet<QByteArray> changedParts;
    QSet<QByteArray> dummyBa;
    QSet<qint64> dummyIv;

    auto msg = Protocol::CollectionChangeNotificationPtr::create();

    if (version == 1) {
        stream >> operation;
        stream >> uid;
        stream >> remoteId;
        stream >> resource;
        stream >> parentCollection;
        stream >> parentDestCollection;
        stream >> dummyString;
        stream >> changedParts;

        Protocol::FetchCollectionsResponse collection;
        collection.setId(uid);
        collection.setRemoteId(remoteId);
        msg->setCollection(std::move(collection));
        msg->addMetadata("FETCH_COLLECTION");
    } else if (version >= 2) {
        stream >> operation;
        stream >> entityCnt;
        if (version >= 7) {
            QString str;
            QStringList stringList;
            qint64 i64;
            QList<qint64> vb;
            QMap<QByteArray, QByteArray> attrs;
            bool b;
            int i;
            Tristate tristate;
            Protocol::FetchCollectionsResponse collection;
            stream >> uid;
            collection.setId(uid);
            stream >> uid;
            collection.setParentId(uid);
            stream >> str;
            collection.setName(str);
            stream >> stringList;
            collection.setMimeTypes(stringList);
            stream >> str;
            collection.setRemoteId(str);
            stream >> str;
            collection.setRemoteRevision(str);
            stream >> str;
            collection.setResource(str);

            Protocol::FetchCollectionStatsResponse stats;
            stream >> i64;
            stats.setCount(i64);
            stream >> i64;
            stats.setUnseen(i64);
            stream >> i64;
            stats.setSize(i64);
            collection.setStatistics(stats);

            stream >> str;
            collection.setSearchQuery(str);
            stream >> vb;
            collection.setSearchCollections(vb);
            stream >> entityCnt;
            QList<Protocol::Ancestor> ancestors;
            for (int j = 0; j < entityCnt; ++j) {
                Protocol::Ancestor ancestor;
                stream >> i64;
                ancestor.setId(i64);
                stream >> str;
                ancestor.setRemoteId(str);
                stream >> str;
                ancestor.setName(str);
                stream >> attrs;
                ancestor.setAttributes(attrs);
                ancestors.push_back(ancestor);

                if (stream.status() != QDataStream::Ok) {
                    qCWarning(AKONADICORE_LOG) << "Erorr reading saved notifications! Aborting";
                    return msg;
                }
            }
            collection.setAncestors(ancestors);

            Protocol::CachePolicy cachePolicy;
            stream >> b;
            cachePolicy.setInherit(b);
            stream >> i;
            cachePolicy.setCheckInterval(i);
            stream >> i;
            cachePolicy.setCacheTimeout(i);
            stream >> b;
            cachePolicy.setSyncOnDemand(b);
            stream >> stringList;
            cachePolicy.setLocalParts(stringList);
            collection.setCachePolicy(cachePolicy);

            stream >> attrs;
            collection.setAttributes(attrs);
            stream >> b;
            collection.setEnabled(b);
            stream >> reinterpret_cast<qint8 &>(tristate);
            collection.setDisplayPref(tristate);
            stream >> reinterpret_cast<qint8 &>(tristate);
            collection.setSyncPref(tristate);
            stream >> reinterpret_cast<qint8 &>(tristate);
            collection.setIndexPref(tristate);
            stream >> b; // read the deprecated "isReferenced" value
            stream >> b;
            collection.setIsVirtual(b);

            msg->setCollection(std::move(collection));
        } else {
            for (int j = 0; j < entityCnt; ++j) {
                stream >> uid;
                stream >> remoteId;
                stream >> remoteRevision;
                stream >> dummyString;
                if (stream.status() != QDataStream::Ok) {
                    qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting";
                    return msg;
                }
                Protocol::FetchCollectionsResponse collection;
                collection.setId(uid);
                collection.setRemoteId(remoteId);
                collection.setRemoteRevision(remoteRevision);
                msg->setCollection(std::move(collection));
                msg->addMetadata("FETCH_COLLECTION");
            }
        }
        stream >> resource;
        stream >> destinationResource;
        stream >> parentCollection;
        stream >> parentDestCollection;
        stream >> changedParts;
        stream >> dummyBa;
        stream >> dummyBa;
        if (version >= 3) {
            stream >> dummyIv;
            stream >> dummyIv;
        }
    } else {
        qCWarning(AKONADICORE_LOG) << "Error version is not correct here" << version;
        return msg;
    }

    if (version >= 5) {
        msg->setOperation(static_cast<Protocol::CollectionChangeNotification::Operation>(operation));
    } else {
        msg->setOperation(mapCollectionOperation(static_cast<LegacyOp>(operation)));
    }
    msg->setResource(resource);
    msg->setDestinationResource(destinationResource);
    msg->setParentCollection(parentCollection);
    msg->setParentDestCollection(parentDestCollection);
    msg->setChangedParts(changedParts);
    return msg;
}

void Akonadi::ChangeRecorderJournalWriter::saveCollectionNotification(QDataStream &stream, const Protocol::CollectionChangeNotification &msg)
{
    // Version 7

    const auto &col = msg.collection();

    stream << int(msg.operation());
    stream << int(1);
    stream << col.id();
    stream << col.parentId();
    stream << col.name();
    stream << col.mimeTypes();
    stream << col.remoteId();
    stream << col.remoteRevision();
    stream << col.resource();
    const auto stats = col.statistics();
    stream << stats.count();
    stream << stats.unseen();
    stream << stats.size();
    stream << col.searchQuery();
    stream << col.searchCollections();
    const auto ancestors = col.ancestors();
    stream << static_cast<int>(ancestors.count());
    for (const auto &ancestor : ancestors) {
        stream << ancestor.id() << ancestor.remoteId() << ancestor.name() << ancestor.attributes();
    }
    const auto cachePolicy = col.cachePolicy();
    stream << cachePolicy.inherit();
    stream << cachePolicy.checkInterval();
    stream << cachePolicy.cacheTimeout();
    stream << cachePolicy.syncOnDemand();
    stream << cachePolicy.localParts();
    stream << col.attributes();
    stream << col.enabled();
    stream << static_cast<qint8>(col.displayPref());
    stream << static_cast<qint8>(col.syncPref());
    stream << static_cast<qint8>(col.indexPref());
    stream << false; // write the deprecated "isReferenced" value
    stream << col.isVirtual();

    stream << msg.resource();
    stream << msg.destinationResource();
    stream << quint64(msg.parentCollection());
    stream << quint64(msg.parentDestCollection());
    stream << msg.changedParts();
    stream << QSet<QByteArray>();
    stream << QSet<QByteArray>();
    stream << QSet<qint64>();
    stream << QSet<qint64>();
}

Protocol::ChangeNotificationPtr ChangeRecorderJournalReader::loadTagNotification(QDataStream &stream, quint64 version)
{
    QByteArray resource;
    QByteArray dummyBa;
    int operation;
    int entityCnt;
    quint64 uid;
    quint64 dummyI;
    QString remoteId;
    QString dummyString;
    QSet<QByteArray> dummyBaV;
    QSet<qint64> dummyIv;

    auto msg = Protocol::TagChangeNotificationPtr::create();

    if (version == 1) {
        stream >> operation;
        stream >> uid;
        stream >> remoteId;
        stream >> dummyBa;
        stream >> dummyI;
        stream >> dummyI;
        stream >> dummyString;
        stream >> dummyBaV;

        Protocol::FetchTagsResponse tag;
        tag.setId(uid);
        tag.setRemoteId(remoteId.toLatin1());
        msg->setTag(std::move(tag));
        msg->addMetadata("FETCH_TAG");
    } else if (version >= 2) {
        stream >> operation;
        stream >> entityCnt;
        if (version >= 7) {
            QByteArray ba;
            QMap<QByteArray, QByteArray> attrs;

            Protocol::FetchTagsResponse tag;

            stream >> uid;
            tag.setId(uid);
            stream >> ba;
            tag.setParentId(uid);
            stream >> attrs;
            tag.setGid(ba);
            stream >> ba;
            tag.setType(ba);
            stream >> uid;
            tag.setRemoteId(ba);
            stream >> ba;
            tag.setAttributes(attrs);
            msg->setTag(std::move(tag));

            stream >> resource;
        } else {
            for (int j = 0; j < entityCnt; ++j) {
                stream >> uid;
                stream >> remoteId;
                stream >> dummyString;
                stream >> dummyString;
                if (stream.status() != QDataStream::Ok) {
                    qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting";
                    return msg;
                }
                Protocol::FetchTagsResponse tag;
                tag.setId(uid);
                tag.setRemoteId(remoteId.toLatin1());
                msg->setTag(std::move(tag));
                msg->addMetadata("FETCH_TAG");
            }
            stream >> resource;
            stream >> dummyBa;
            stream >> dummyI;
            stream >> dummyI;
            stream >> dummyBaV;
            stream >> dummyBaV;
            stream >> dummyBaV;
            if (version >= 3) {
                stream >> dummyIv;
                stream >> dummyIv;
            }
        }
        if (version >= 5) {
            msg->setOperation(static_cast<Protocol::TagChangeNotification::Operation>(operation));
        } else {
            msg->setOperation(mapTagOperation(static_cast<LegacyOp>(operation)));
        }
    }
    msg->setResource(resource);
    return msg;
}

void Akonadi::ChangeRecorderJournalWriter::saveTagNotification(QDataStream &stream, const Protocol::TagChangeNotification &msg)
{
    const auto &tag = msg.tag();
    stream << int(msg.operation());
    stream << int(1);
    stream << tag.id();
    stream << tag.parentId();
    stream << tag.gid();
    stream << tag.type();
    stream << tag.remoteId();
    stream << tag.attributes();
    stream << msg.resource();
}

Protocol::ChangeNotificationPtr ChangeRecorderJournalReader::loadRelationNotification(QDataStream &stream, quint64 version)
{
    // NOTE: While relations have been deprecated and removed from Akonadi, we still need to support reading them from
    // the journal, otherwise we would not be able to read old journals.

    QByteArray dummyBa;
    int operation;
    int entityCnt;
    quint64 dummyI;
    QString dummyString;
    QSet<QByteArray> itemParts;
    QSet<QByteArray> dummyBaV;
    QSet<qint64> dummyIv;

    if (version == 1) {
        qCWarning(AKONADICORE_LOG) << "Invalid version of relation notification";
        return {};
    } else if (version >= 2) {
        stream >> operation;
        stream >> entityCnt;
        if (version >= 7) {
            stream >> dummyI; // left
            stream >> dummyBa; // left mimetype
            stream >> dummyI; // right
            stream >> dummyBa; // right mimetype
            stream >> dummyBa; // remoteid
            stream >> dummyBa; // type
        } else {
            for (int j = 0; j < entityCnt; ++j) {
                stream >> dummyI;
                stream >> dummyString;
                stream >> dummyString;
                stream >> dummyString;
                if (stream.status() != QDataStream::Ok) {
                    qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting";
                    return {};
                }
            }
            stream >> dummyBa;
            if (version == 5) {
                // there was a bug in version 5 serializer that serialized this
                // field as qint64 (8 bytes) instead of empty QByteArray (which is
                // 4 bytes)
                stream >> dummyI;
            } else {
                stream >> dummyBa;
            }
            stream >> dummyI;
            stream >> dummyI;
            stream >> itemParts;
            stream >> dummyBaV;
            stream >> dummyBaV;
            if (version >= 3) {
                stream >> dummyIv;
                stream >> dummyIv;
            }
        }
    }

    return {};
}

Protocol::ItemChangeNotification::Operation ChangeRecorderJournalReader::mapItemOperation(LegacyOp op)
{
    switch (op) {
    case Add:
        return Protocol::ItemChangeNotification::Add;
    case Modify:
        return Protocol::ItemChangeNotification::Modify;
    case Move:
        return Protocol::ItemChangeNotification::Move;
    case Remove:
        return Protocol::ItemChangeNotification::Remove;
    case Link:
        return Protocol::ItemChangeNotification::Link;
    case Unlink:
        return Protocol::ItemChangeNotification::Unlink;
    case ModifyFlags:
        return Protocol::ItemChangeNotification::ModifyFlags;
    case ModifyTags:
        return Protocol::ItemChangeNotification::ModifyTags;
    case ModifyRelations:
        [[fallthrough]];
    default:
        qWarning() << "Unexpected operation type in item notification";
        return Protocol::ItemChangeNotification::InvalidOp;
    }
}

Protocol::CollectionChangeNotification::Operation ChangeRecorderJournalReader::mapCollectionOperation(LegacyOp op)
{
    switch (op) {
    case Add:
        return Protocol::CollectionChangeNotification::Add;
    case Modify:
        return Protocol::CollectionChangeNotification::Modify;
    case Move:
        return Protocol::CollectionChangeNotification::Move;
    case Remove:
        return Protocol::CollectionChangeNotification::Remove;
    case Subscribe:
        return Protocol::CollectionChangeNotification::Subscribe;
    case Unsubscribe:
        return Protocol::CollectionChangeNotification::Unsubscribe;
    default:
        qCWarning(AKONADICORE_LOG) << "Unexpected operation type in collection notification";
        return Protocol::CollectionChangeNotification::InvalidOp;
    }
}

Protocol::TagChangeNotification::Operation ChangeRecorderJournalReader::mapTagOperation(LegacyOp op)
{
    switch (op) {
    case Add:
        return Protocol::TagChangeNotification::Add;
    case Modify:
        return Protocol::TagChangeNotification::Modify;
    case Remove:
        return Protocol::TagChangeNotification::Remove;
    default:
        qCWarning(AKONADICORE_LOG) << "Unexpected operation type in tag notification";
        return Protocol::TagChangeNotification::InvalidOp;
    }
}

ChangeRecorderJournalReader::LegacyType ChangeRecorderJournalWriter::mapToLegacyType(Protocol::Command::Type type)
{
    switch (type) {
    case Protocol::Command::ItemChangeNotification:
        return ChangeRecorderJournalReader::Item;
    case Protocol::Command::CollectionChangeNotification:
        return ChangeRecorderJournalReader::Collection;
    case Protocol::Command::TagChangeNotification:
        return ChangeRecorderJournalReader::Tag;
    default:
        qCWarning(AKONADICORE_LOG) << "Unexpected notification type";
        return ChangeRecorderJournalReader::InvalidType;
    }
}
