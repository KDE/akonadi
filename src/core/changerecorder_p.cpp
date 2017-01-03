/*
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

#include "changerecorder_p.h"
#include "akonadicore_debug.h"

#include <QFile>
#include <QDir>
#include <QSettings>
#include <QFileInfo>
#include <QDataStream>

using namespace Akonadi;

ChangeRecorderPrivate::ChangeRecorderPrivate(ChangeNotificationDependenciesFactory *dependenciesFactory_,
        ChangeRecorder *parent)
    : MonitorPrivate(dependenciesFactory_, parent)
    , settings(0)
    , enableChangeRecording(true)
    , m_lastKnownNotificationsCount(0)
    , m_startOffset(0)
    , m_needFullSave(true)
{
}

int ChangeRecorderPrivate::pipelineSize() const
{
    if (enableChangeRecording) {
        return 0; // we fill the pipeline ourselves when using change recording
    }
    return MonitorPrivate::pipelineSize();
}

void ChangeRecorderPrivate::slotNotify(const Akonadi::Protocol::ChangeNotification &msg)
{
    Q_Q(ChangeRecorder);
    const int oldChanges = pendingNotifications.size();
    // with change recording disabled this will automatically take care of dispatching notification messages and saving
    MonitorPrivate::slotNotify(msg);
    if (enableChangeRecording && pendingNotifications.size() != oldChanges) {
        emit q->changesAdded();
    }
}

// The QSettings object isn't actually used anymore, except for migrating old data
// and it gives us the base of the filename to use. This is all historical.
QString ChangeRecorderPrivate::notificationsFileName() const
{
    return settings->fileName() + QStringLiteral("_changes.dat");
}

void ChangeRecorderPrivate::loadNotifications()
{
    pendingNotifications.clear();
    Q_ASSERT(pipeline.isEmpty());
    pipeline.clear();

    const QString changesFileName = notificationsFileName();

    /**
     * In an older version we recorded changes inside the settings object, however
     * for performance reasons we changed that to store them in a separated file.
     * If this file doesn't exists, it means we run the new version the first time,
     * so we have to read in the legacy list of changes first.
     */
    if (!QFile::exists(changesFileName)) {
        QStringList list;
        settings->beginGroup(QStringLiteral("ChangeRecorder"));
        const int size = settings->beginReadArray(QStringLiteral("change"));

        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            Protocol::ChangeNotification msg;

            switch (static_cast<LegacyType>(settings->value(QStringLiteral("type")).toInt())) {
            case Item:
                msg = loadItemNotification(settings);
                break;
            case Collection:
                msg = loadCollectionNotification(settings);
            case Tag:
            case Relation:
            case InvalidType:
                qWarning() << "Unexpected notification type in legacy store";
                continue;
            }
            if (msg.isValid()) {
                pendingNotifications << msg;
            }
        }

        settings->endArray();

        // save notifications to the new file...
        saveNotifications();

        // ...delete the legacy list...
        settings->remove(QString());
        settings->endGroup();

        // ...and continue as usually
    }

    QFile file(changesFileName);
    if (file.open(QIODevice::ReadOnly)) {
        m_needFullSave = false;
        pendingNotifications = loadFrom(&file, m_needFullSave);
    } else {
        m_needFullSave = true;
    }
    notificationsLoaded();
}

static const quint64 s_currentVersion = Q_UINT64_C(0x000600000000);
static const quint64 s_versionMask    = Q_UINT64_C(0xFFFF00000000);
static const quint64 s_sizeMask       = Q_UINT64_C(0x0000FFFFFFFF);

QQueue<Protocol::ChangeNotification> ChangeRecorderPrivate::loadFrom(QFile *device, bool &needsFullSave) const
{
    QDataStream stream(device);
    stream.setVersion(QDataStream::Qt_4_6);

    QByteArray sessionId;
    int type;

    QQueue<Protocol::ChangeNotification> list;

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
        Protocol::ChangeNotification msg;
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
            msg = loadRelationNotification(stream, version);
            break;
        default:
            qWarning() << "Unknown notification type";
            break;
        }

        if (i < startOffset) {
            continue;
        }

        if (msg.isValid()) {
            msg.setSessionId(sessionId);
            list << msg;
        }
    }

    return list;
}

QString ChangeRecorderPrivate::dumpNotificationListToString() const
{
    if (!settings) {
        return QStringLiteral("No settings set in ChangeRecorder yet.");
    }
    const QString changesFileName = notificationsFileName();
    QFile file(changesFileName);

    if (!file.open(QIODevice::ReadOnly)) {
        return QLatin1String("Error reading ") + changesFileName;
    }

    QString result;
    bool dummy;
    const QQueue<Protocol::ChangeNotification> notifications = loadFrom(&file, dummy);
    Q_FOREACH (const Protocol::ChangeNotification &n, notifications) {
        result += n.debugString() + QLatin1Char('\n');
    }
    return result;
}

void ChangeRecorderPrivate::addToStream(QDataStream &stream, const Protocol::ChangeNotification &msg)
{
    // We deliberately don't use Factory::serialize(), because the internal
    // serialization format could change at any point

    stream << msg.sessionId();
    stream << int(mapToLegacyType(msg.type()));
    switch (msg.type()) {
    case Protocol::Command::ItemChangeNotification:
        saveItemNotification(stream, static_cast<const Protocol::ItemChangeNotification&>(msg));
        break;
    case Protocol::Command::CollectionChangeNotification:
        saveCollectionNotification(stream, static_cast<const Protocol::CollectionChangeNotification&>(msg));
        break;
    case Protocol::Command::TagChangeNotification:
        saveTagNotification(stream, static_cast<const Protocol::TagChangeNotification&>(msg));
        break;
    case Protocol::Command::RelationChangeNotification:
        saveRelationNotification(stream, static_cast<const Protocol::RelationChangeNotification&>(msg));
        break;
    default:
        qWarning() << "Unexpected type?";
        return;
    }
}

void ChangeRecorderPrivate::writeStartOffset()
{
    if (!settings) {
        return;
    }

    QFile file(notificationsFileName());
    if (!file.open(QIODevice::ReadWrite)) {
        qCWarning(AKONADICORE_LOG) << "Could not update notifications in file" << file.fileName();
        return;
    }

    // Skip "countAndVersion"
    file.seek(8);

    //qCDebug(AKONADICORE_LOG) << "Writing start offset=" << m_startOffset;

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_4_6);
    stream << static_cast<quint64>(m_startOffset);

    // Everything else stays unchanged
}

void ChangeRecorderPrivate::saveNotifications()
{
    if (!settings) {
        return;
    }

    QFile file(notificationsFileName());
    QFileInfo info(file);
    if (!QFile::exists(info.absolutePath())) {
        QDir dir;
        dir.mkpath(info.absolutePath());
    }
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(AKONADICORE_LOG) << "Could not save notifications to file" << file.fileName();
        return;
    }
    saveTo(&file);
    m_needFullSave = false;
    m_startOffset = 0;
}

void ChangeRecorderPrivate::saveTo(QIODevice *device)
{
    // Version 0 of this file format was writing a quint64 count, followed by the notifications.
    // Version 1 bundles a version number into that quint64, to be able to detect a version number at load time.

    const quint64 countAndVersion = static_cast<quint64>(pendingNotifications.count()) | s_currentVersion;

    QDataStream stream(device);
    stream.setVersion(QDataStream::Qt_4_6);

    stream << countAndVersion;
    stream << quint64(0); // no start offset

    //qCDebug(AKONADICORE_LOG) << "Saving" << pendingNotifications.count() << "notifications (full save)";

    for (int i = 0; i < pendingNotifications.count(); ++i) {
        const Protocol::ChangeNotification msg = pendingNotifications.at(i);
        addToStream(stream, msg);
    }
}

void ChangeRecorderPrivate::notificationsEnqueued(int count)
{
    // Just to ensure the contract is kept, and these two methods are always properly called.
    if (enableChangeRecording) {
        m_lastKnownNotificationsCount += count;
        if (m_lastKnownNotificationsCount != pendingNotifications.count()) {
            qCWarning(AKONADICORE_LOG) << this << "The number of pending notifications changed without telling us! Expected"
                       << m_lastKnownNotificationsCount << "but got" << pendingNotifications.count()
                       << "Caller just added" << count;
            Q_ASSERT(pendingNotifications.count() == m_lastKnownNotificationsCount);
        }

        saveNotifications();
    }
}

void ChangeRecorderPrivate::dequeueNotification()
{
    if (pendingNotifications.isEmpty()) {
        return;
    }

    pendingNotifications.dequeue();
    if (enableChangeRecording) {

        Q_ASSERT(pendingNotifications.count() == m_lastKnownNotificationsCount - 1);
        --m_lastKnownNotificationsCount;

        if (m_needFullSave || pendingNotifications.isEmpty()) {
            saveNotifications();
        } else {
            ++m_startOffset;
            writeStartOffset();
        }
    }
}

void ChangeRecorderPrivate::notificationsErased()
{
    if (enableChangeRecording) {
        m_lastKnownNotificationsCount = pendingNotifications.count();
        m_needFullSave = true;
        saveNotifications();
    }
}

void ChangeRecorderPrivate::notificationsLoaded()
{
    m_lastKnownNotificationsCount = pendingNotifications.count();
    m_startOffset = 0;
}

bool ChangeRecorderPrivate::emitNotification(const Protocol::ChangeNotification &msg)
{
    const bool someoneWasListening = MonitorPrivate::emitNotification(msg);
    if (!someoneWasListening && enableChangeRecording) {
        //If no signal was emitted (e.g. because no one was connected to it), no one is going to call changeProcessed, so we help ourselves.
        dequeueNotification();
        QMetaObject::invokeMethod(q_ptr, "replayNext", Qt::QueuedConnection);
    }
    return someoneWasListening;
}


Protocol::ChangeNotification ChangeRecorderPrivate::loadItemNotification(QSettings* settings) const
{
    Protocol::ItemChangeNotification msg;
    msg.setSessionId(settings->value(QStringLiteral("sessionId")).toByteArray());
    msg.setOperation(mapItemOperation(static_cast<LegacyOp>(settings->value(QStringLiteral("op")).toInt())));
    msg.addItem(settings->value(QStringLiteral("uid")).toLongLong(),
                settings->value(QStringLiteral("rid")).toString(),
                QString(),
                settings->value(QStringLiteral("mimeType")).toString());
    msg.setResource(settings->value(QStringLiteral("resource")).toByteArray());
    msg.setParentCollection(settings->value(QStringLiteral("parentCol")).toLongLong());
    msg.setParentDestCollection(settings->value(QStringLiteral("parentDestCol")).toLongLong());
    const QStringList list = settings->value(QStringLiteral("itemParts")).toStringList();
    QSet<QByteArray> itemParts;
    Q_FOREACH (const QString &entry, list) {
        itemParts.insert(entry.toLatin1());
    }
    msg.setItemParts(itemParts);
    return msg;
}


Protocol::ChangeNotification ChangeRecorderPrivate::loadCollectionNotification(QSettings* settings) const
{
    Protocol::CollectionChangeNotification msg;
    msg.setSessionId(settings->value(QStringLiteral("sessionId")).toByteArray());
    msg.setOperation(mapCollectionOperation(static_cast<LegacyOp>(settings->value(QStringLiteral("op")).toInt())));
    msg.setId(settings->value(QStringLiteral("uid")).toLongLong());
    msg.setRemoteId(settings->value(QStringLiteral("rid")).toString());
    msg.setResource(settings->value(QStringLiteral("resource")).toByteArray());
    msg.setParentCollection(settings->value(QStringLiteral("parentCol")).toLongLong());
    msg.setParentDestCollection(settings->value(QStringLiteral("parentDestCol")).toLongLong());
    const QStringList list = settings->value(QStringLiteral("itemParts")).toStringList();
    QSet<QByteArray> changedParts;
    Q_FOREACH (const QString &entry, list) {
        changedParts.insert(entry.toLatin1());
    }
    msg.setChangedParts(changedParts);
    return msg;
}

QSet<Protocol::ItemChangeNotification::Relation> ChangeRecorderPrivate::extractRelations(QSet<QByteArray> &flags) const
{
    QSet<Protocol::ItemChangeNotification::Relation> relations;
    auto iter = flags.begin();
    while (iter != flags.end()) {
        if (iter->startsWith("RELATION")) {
            const QByteArrayList parts = iter->split(' ');
            Q_ASSERT(parts.size() == 4);
            Protocol::ItemChangeNotification::Relation relation;
            relation.type = QString::fromLatin1(parts[1]);
            relation.leftId = parts[2].toLongLong();
            relation.rightId = parts[3].toLongLong();
            relations.insert(relation);
            iter = flags.erase(iter);
        } else {
            ++iter;
        }
    }

    return relations;
}


Protocol::ChangeNotification ChangeRecorderPrivate::loadItemNotification(QDataStream &stream, quint64 version) const
{
    QByteArray resource, destinationResource;
    int operation, entityCnt;
    quint64 uid, parentCollection, parentDestCollection;
    QString remoteId, mimeType, remoteRevision;
    QSet<QByteArray> itemParts, addedFlags, removedFlags;
    QSet<qint64> addedTags, removedTags;

    Protocol::ItemChangeNotification msg;

    if (version == 1) {
        stream >> operation;
        stream >> uid;
        stream >> remoteId;
        stream >> resource;
        stream >> parentCollection;
        stream >> parentDestCollection;
        stream >> mimeType;
        stream >> itemParts;

        msg.addItem(uid, remoteId, QString(), mimeType);
    } else if (version >= 2) {
        stream >> operation;
        stream >> entityCnt;
        for (int j = 0; j < entityCnt; ++j) {
            stream >> uid;
            stream >> remoteId;
            stream >> remoteRevision;
            stream >> mimeType;
            if (stream.status() != QDataStream::Ok) {
                qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting";
                return msg;
            }
            msg.addItem(uid, remoteId, remoteRevision, mimeType);
        }
        stream >> resource;
        stream >> destinationResource;
        stream >> parentCollection;
        stream >> parentDestCollection;
        stream >> itemParts;
        stream >> addedFlags;
        stream >> removedFlags;
        if (version >= 3) {
            stream >> addedTags;
            stream >> removedTags;
        }
    }
    if (version >= 5) {
        msg.setOperation(static_cast<Protocol::ItemChangeNotification::Operation>(operation));
    } else {
        msg.setOperation(mapItemOperation(static_cast<LegacyOp>(operation)));
    }
    msg.setResource(resource);
    msg.setDestinationResource(destinationResource);
    msg.setParentCollection(parentCollection);
    msg.setParentDestCollection(parentDestCollection);
    msg.setItemParts(itemParts);
    msg.setAddedRelations(extractRelations(addedFlags));
    msg.setAddedFlags(addedFlags);
    msg.setRemovedRelations(extractRelations(removedFlags));
    msg.setRemovedFlags(removedFlags);
    msg.setAddedTags(addedTags);
    msg.setRemovedTags(removedTags);
    return msg;
}

QSet<QByteArray> ChangeRecorderPrivate::encodeRelations(const QSet<Protocol::ItemChangeNotification::Relation> &relations) const
{
    QSet<QByteArray> rv;
    Q_FOREACH (const auto &rel, relations) {
        rv.insert("RELATION " + rel.type.toLatin1() + ' ' + QByteArray::number(rel.leftId) + ' ' + QByteArray::number(rel.rightId));
    }
    return rv;
}

void ChangeRecorderPrivate::saveItemNotification(QDataStream &stream, const Protocol::ItemChangeNotification &msg)
{
    stream << int(msg.operation());
    const auto items = msg.items();
    stream << items.count();
    Q_FOREACH (const Protocol::ItemChangeNotification::Item &item, items) {
        stream << quint64(item.id);
        stream << item.remoteId;
        stream << item.remoteRevision;
        stream << item.mimeType;
    }
    stream << msg.resource();
    stream << msg.destinationResource();
    stream << quint64(msg.parentCollection());
    stream << quint64(msg.parentDestCollection());
    stream << msg.itemParts();
    stream << msg.addedFlags() + encodeRelations(msg.addedRelations());
    stream << msg.removedFlags() + encodeRelations(msg.removedRelations());
    stream << msg.addedTags();
    stream << msg.removedTags();
}

Protocol::ChangeNotification ChangeRecorderPrivate::loadCollectionNotification(QDataStream &stream, quint64 version) const
{
    QByteArray resource, destinationResource;
    int operation, entityCnt;
    quint64 uid, parentCollection, parentDestCollection;
    QString remoteId, remoteRevision, dummyString;
    QSet<QByteArray> changedParts, dummyBa;
    QSet<qint64> dummyIv;

    Protocol::CollectionChangeNotification msg;

    if (version == 1) {
        stream >> operation;
        stream >> uid;
        stream >> remoteId;
        stream >> resource;
        stream >> parentCollection;
        stream >> parentDestCollection;
        stream >> dummyString;
        stream >> changedParts;

        msg.setId(uid);
        msg.setRemoteId(remoteId);
    } else if (version >= 2) {
        stream >> operation;
        stream >> entityCnt;
        for (int j = 0; j < entityCnt; ++j) {
            stream >> uid;
            stream >> remoteId;
            stream >> remoteRevision;
            stream >> dummyString;
            if (stream.status() != QDataStream::Ok) {
                qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting";
                return msg;
            }
            msg.setId(uid);
            msg.setRemoteId(remoteId);
            msg.setRemoteRevision(remoteRevision);
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
    }
    if (version >= 5) {
        msg.setOperation(static_cast<Protocol::CollectionChangeNotification::Operation>(operation));
    } else {
        msg.setOperation(mapCollectionOperation(static_cast<LegacyOp>(operation)));
    }
    msg.setResource(resource);
    msg.setDestinationResource(destinationResource);
    msg.setParentCollection(parentCollection);
    msg.setParentDestCollection(parentDestCollection);
    msg.setChangedParts(changedParts);
    return msg;
}

void Akonadi::ChangeRecorderPrivate::saveCollectionNotification(QDataStream& stream, const Protocol::CollectionChangeNotification &msg)
{
    stream << int(msg.operation());
    stream << int(1);
    stream << msg.id();
    stream << msg.remoteId();
    stream << msg.remoteRevision();
    stream << QString();
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

Protocol::ChangeNotification ChangeRecorderPrivate::loadTagNotification(QDataStream &stream, quint64 version) const
{
    QByteArray resource, dummyBa;
    int operation, entityCnt;
    quint64 uid, dummyI;
    QString remoteId, dummyString;
    QSet<QByteArray> dummyBaV;
    QSet<qint64> dummyIv;

    Protocol::TagChangeNotification msg;

    if (version == 1) {
        stream >> operation;
        stream >> uid;
        stream >> remoteId;
        stream >> dummyBa;
        stream >> dummyI;
        stream >> dummyI;
        stream >> dummyString;
        stream >> dummyBaV;

        msg.setId(uid);
        msg.setRemoteId(remoteId);
    } else if (version >= 2) {
        stream >> operation;
        stream >> entityCnt;
        for (int j = 0; j < entityCnt; ++j) {
            stream >> uid;
            stream >> remoteId;
            stream >> dummyString;
            stream >> dummyString;
            if (stream.status() != QDataStream::Ok) {
                qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting";
                return msg;
            }
            msg.setId(uid);
            msg.setRemoteId(remoteId);
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
        msg.setOperation(static_cast<Protocol::TagChangeNotification::Operation>(operation));
    } else {
        msg.setOperation(mapTagOperation(static_cast<LegacyOp>(operation)));
    }
    msg.setResource(resource);
    return msg;
}

void Akonadi::ChangeRecorderPrivate::saveTagNotification(QDataStream& stream, const Protocol::TagChangeNotification &msg)
{
    stream << int(msg.operation());
    stream << int(1);
    stream << msg.id();
    stream << msg.remoteId();
    stream << QString();
    stream << QString();
    stream << msg.resource();
    stream << qint64(0);
    stream << qint64(0);
    stream << qint64(0);
    stream << QSet<QByteArray>();
    stream << QSet<QByteArray>();
    stream << QSet<QByteArray>();
    stream << QSet<qint64>();
    stream << QSet<qint64>();
}

Protocol::ChangeNotification ChangeRecorderPrivate::loadRelationNotification(QDataStream &stream, quint64 version) const
{
    QByteArray dummyBa;
    int operation, entityCnt;
    quint64 dummyI;
    QString dummyString;
    QSet<QByteArray> itemParts, dummyBaV;
    QSet<qint64> dummyIv;

    Protocol::RelationChangeNotification msg;

    if (version == 1) {
        qWarning() << "Invalid version of relation notification";
        return msg;
    } else if (version >= 2) {
        stream >> operation;
        stream >> entityCnt;
        for (int j = 0; j < entityCnt; ++j) {
            stream >> dummyI;
            stream >> dummyString;
            stream >> dummyString;
            stream >> dummyString;
            if (stream.status() != QDataStream::Ok) {
                qCWarning(AKONADICORE_LOG) << "Error reading saved notifications! Aborting";
                return msg;
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

    if (version >= 5) {
        msg.setOperation(static_cast<Protocol::RelationChangeNotification::Operation>(operation));
    } else {
        msg.setOperation(mapRelationOperation(static_cast<LegacyOp>(operation)));
    }
    Q_FOREACH (const QByteArray &part, itemParts) {
        const QByteArrayList p = part.split(' ');
        if (p.size() < 2) {
            continue;
        }
        if (p[0] == "LEFT") {
            msg.setLeftItem(p[1].toLongLong());
        } else if (p[0] == "RIGHT") {
            msg.setRightItem(p[1].toLongLong());
        } else if (p[0] == "RID") {
            msg.setRemoteId(QString::fromLatin1(p[1]));
        } else if (p[0] == "TYPE") {
            msg.setType(QString::fromLatin1(p[1]));
        }
    }
    return msg;
}

void Akonadi::ChangeRecorderPrivate::saveRelationNotification(QDataStream& stream, const Protocol::RelationChangeNotification &msg)
{
    QSet<QByteArray> rv;
    rv.insert("LEFT " + QByteArray::number(msg.leftItem()));
    rv.insert("RIGHT " + QByteArray::number(msg.rightItem()));
    rv.insert("RID " + msg.remoteId().toLatin1());
    rv.insert("TYPE " + msg.type().toLatin1());

    stream << int(msg.operation());
    stream << int(0);
    stream << qint64(0);
    stream << QString();
    stream << QString();
    stream << QString();
    stream << QByteArray();
    stream << QByteArray();
    stream << qint64(0);
    stream << qint64(0);
    stream << rv;
    stream << QSet<QByteArray>();
    stream << QSet<QByteArray>();
    stream << QSet<qint64>();
    stream << QSet<qint64>();
}

Protocol::ItemChangeNotification::Operation ChangeRecorderPrivate::mapItemOperation(LegacyOp op) const
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
        return Protocol::ItemChangeNotification::ModifyRelations;
    default:
        qWarning() << "Unexpected operation type in item notification";
        return Protocol::ItemChangeNotification::InvalidOp;
    }
}

Protocol::CollectionChangeNotification::Operation ChangeRecorderPrivate::mapCollectionOperation(LegacyOp op) const
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
        qWarning() << "Unexpected operation type in collection notification";
        return Protocol::CollectionChangeNotification::InvalidOp;
    }
}

Protocol::TagChangeNotification::Operation ChangeRecorderPrivate::mapTagOperation(LegacyOp op) const
{
    switch (op) {
    case Add:
        return Protocol::TagChangeNotification::Add;
    case Modify:
        return Protocol::TagChangeNotification::Modify;
    case Remove:
        return Protocol::TagChangeNotification::Remove;
    default:
        qWarning() << "Unexpected operation type in tag notification";
        return Protocol::TagChangeNotification::InvalidOp;
    }
}

Protocol::RelationChangeNotification::Operation ChangeRecorderPrivate::mapRelationOperation(LegacyOp op) const
{
    switch (op) {
    case Add:
        return Protocol::RelationChangeNotification::Add;
    case Remove:
        return Protocol::RelationChangeNotification::Remove;
    default:
        qWarning() << "Unexpected operation type in relation notification";
        return Protocol::RelationChangeNotification::InvalidOp;
    }
}

ChangeRecorderPrivate::LegacyType ChangeRecorderPrivate::mapToLegacyType(Protocol::Command::Type type) const
{
    switch (type) {
    case Protocol::Command::ItemChangeNotification:
        return Item;
    case Protocol::Command::CollectionChangeNotification:
        return Collection;
    case Protocol::Command::TagChangeNotification:
        return Tag;
    case Protocol::Command::RelationChangeNotification:
        return Relation;
    default:
        qWarning() << "Unexpected notification type";
        return InvalidType;
    }
}
