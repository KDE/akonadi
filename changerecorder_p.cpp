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

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>

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

void ChangeRecorderPrivate::slotNotify(const Akonadi::NotificationMessageV3::List &msgs)
{
    Q_Q(ChangeRecorder);
    const int oldChanges = pendingNotifications.size();
    // with change recording disabled this will automatically take care of dispatching notification messages and saving
    MonitorPrivate::slotNotify(msgs);
    if (enableChangeRecording && pendingNotifications.size() != oldChanges) {
        emit q->changesAdded();
    }
}

// The QSettings object isn't actually used anymore, except for migrating old data
// and it gives us the base of the filename to use. This is all historical.
QString ChangeRecorderPrivate::notificationsFileName() const
{
    return settings->fileName() + QLatin1String("_changes.dat");
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
        settings->beginGroup(QLatin1String("ChangeRecorder"));
        const int size = settings->beginReadArray(QLatin1String("change"));

        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            NotificationMessageV3 msg;
            msg.setSessionId(settings->value(QLatin1String("sessionId")).toByteArray());
            msg.setType((NotificationMessageV2::Type)settings->value(QLatin1String("type")).toInt());
            msg.setOperation((NotificationMessageV2::Operation)settings->value(QLatin1String("op")).toInt());
            msg.addEntity(settings->value(QLatin1String("uid")).toLongLong(),
                          settings->value(QLatin1String("rid")).toString(),
                          QString(),
                          settings->value(QLatin1String("mimeType")).toString());
            msg.setResource(settings->value(QLatin1String("resource")).toByteArray());
            msg.setParentCollection(settings->value(QLatin1String("parentCol")).toLongLong());
            msg.setParentDestCollection(settings->value(QLatin1String("parentDestCol")).toLongLong());
            list = settings->value(QLatin1String("itemParts")).toStringList();
            QSet<QByteArray> itemParts;
            Q_FOREACH (const QString &entry, list) {
                itemParts.insert(entry.toLatin1());
            }
            msg.setItemParts(itemParts);
            pendingNotifications << msg;
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
        pendingNotifications = loadFrom(&file);
    } else {
        m_needFullSave = true;
    }
    notificationsLoaded();
}

static const quint64 s_currentVersion = Q_UINT64_C(0x000300000000);
static const quint64 s_versionMask    = Q_UINT64_C(0xFFFF00000000);
static const quint64 s_sizeMask       = Q_UINT64_C(0x0000FFFFFFFF);

QQueue<NotificationMessageV3> ChangeRecorderPrivate::loadFrom(QIODevice *device)
{
    QDataStream stream(device);
    stream.setVersion(QDataStream::Qt_4_6);

    QByteArray sessionId, resource, destinationResource;
    int type, operation, entityCnt;
    quint64 uid, parentCollection, parentDestCollection;
    QString remoteId, mimeType, remoteRevision;
    QSet<QByteArray> itemParts, addedFlags, removedFlags;
    QSet<qint64> addedTags, removedTags;

    QQueue<NotificationMessageV3> list;

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
    m_needFullSave = startOffset > 0 || version == 0;

    for (quint64 i = 0; i < size && !stream.atEnd(); ++i) {
        NotificationMessageV2 msg;

        if (version == 1) {
            stream >> sessionId;
            stream >> type;
            stream >> operation;
            stream >> uid;
            stream >> remoteId;
            stream >> resource;
            stream >> parentCollection;
            stream >> parentDestCollection;
            stream >> mimeType;
            stream >> itemParts;

            if (i < startOffset) {
                continue;
            }

            msg.setSessionId(sessionId);
            msg.setType(static_cast<NotificationMessageV2::Type>(type));
            msg.setOperation(static_cast<NotificationMessageV2::Operation>(operation));
            msg.addEntity(uid, remoteId, QString(), mimeType);
            msg.setResource(resource);
            msg.setParentCollection(parentCollection);
            msg.setParentDestCollection(parentDestCollection);
            msg.setItemParts(itemParts);

        } else if (version == 2 || version == 3) {

            NotificationMessageV3 msg;

            stream >> sessionId;
            stream >> type;
            stream >> operation;
            stream >> entityCnt;
            for (int j = 0; j < entityCnt; ++j) {
                stream >> uid;
                stream >> remoteId;
                stream >> remoteRevision;
                stream >> mimeType;
                msg.addEntity(uid, remoteId, remoteRevision, mimeType);
            }
            stream >> resource;
            stream >> destinationResource;
            stream >> parentCollection;
            stream >> parentDestCollection;
            stream >> itemParts;
            stream >> addedFlags;
            stream >> removedFlags;
            if (version == 3) {
                stream >> addedTags;
                stream >> removedTags;
            }

            if (i < startOffset) {
                continue;
            }

            msg.setSessionId(sessionId);
            msg.setType(static_cast<NotificationMessageV2::Type>(type));
            msg.setOperation(static_cast<NotificationMessageV2::Operation>(operation));
            msg.setResource(resource);
            msg.setDestinationResource(destinationResource);
            msg.setParentCollection(parentCollection);
            msg.setParentDestCollection(parentDestCollection);
            msg.setItemParts(itemParts);
            msg.setAddedFlags(addedFlags);
            msg.setRemovedFlags(removedFlags);
            msg.setAddedTags(addedTags);
            msg.setRemovedTags(removedTags);

            list << msg;
        }
    }

    return list;
}

QString ChangeRecorderPrivate::dumpNotificationListToString() const
{
    if (!settings) {
        return QString::fromLatin1("No settings set in ChangeRecorder yet.");
    }
    QString result;
    const QString changesFileName = notificationsFileName();
    QFile file(changesFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString::fromLatin1("Error reading ") + changesFileName;
    }

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_4_6);

    QByteArray sessionId, resource, destResource;
    int type, operation, entityCnt;
    quint64 parentCollection, parentDestCollection;
    QString remoteId, remoteRevision, mimeType;
    QSet<QByteArray> itemParts, addedFlags, removedFlags;
    QSet<qint64> addedTags, removedTags;
    QVariantList items;

    QStringList list;

    quint64 sizeAndVersion;
    stream >> sizeAndVersion;

    const quint64 size = sizeAndVersion & s_sizeMask;
    const quint64 version = (sizeAndVersion & s_versionMask) >> 32;

    quint64 startOffset = 0;
    if (version >= 1) {
        stream >> startOffset;
    }

    for (quint64 i = 0; i < size && !stream.atEnd(); ++i) {
        stream >> sessionId;
        stream >> type;
        stream >> operation;
        stream >> entityCnt;
        for (int j = 0; j < entityCnt; ++j) {
            QVariantMap map;
            stream >> map[QLatin1String("uid")];
            stream >> map[QLatin1String("remoteId")];
            stream >> map[QLatin1String("remoteRevision")];
            stream >> map[QLatin1String("mimeType")];
            items << map;
        }
        stream >> resource;
        stream >> destResource;
        stream >> parentCollection;
        stream >> parentDestCollection;
        stream >> itemParts;
        stream >> addedFlags;
        stream >> removedFlags;
        if (version == 3) {
            stream >> addedTags;
            stream >> removedTags;
        }

        if (i < startOffset) {
            continue;
        }

        QString typeString;
        switch (type) {
        case NotificationMessageV2::Collections:
            typeString = QLatin1String("Collections");
            break;
        case NotificationMessageV2::Items:
            typeString = QLatin1String("Items");
            break;
        case NotificationMessageV2::Tags:
            typeString = QLatin1String("Tags");
            break;
        default:
            typeString = QLatin1String("InvalidType");
            break;
        };

        QString operationString;
        switch (operation) {
        case NotificationMessageV2::Add:
            operationString = QLatin1String("Add");
            break;
        case NotificationMessageV2::Modify:
            operationString = QLatin1String("Modify");
            break;
        case NotificationMessageV2::ModifyFlags:
            operationString = QLatin1String("ModifyFlags");
            break;
        case NotificationMessageV2::ModifyTags:
            operationString = QLatin1String("ModifyTags");
            break;
        case NotificationMessageV2::Move:
            operationString = QLatin1String("Move");
            break;
        case NotificationMessageV2::Remove:
            operationString = QLatin1String("Remove");
            break;
        case NotificationMessageV2::Link:
            operationString = QLatin1String("Link");
            break;
        case NotificationMessageV2::Unlink:
            operationString = QLatin1String("Unlink");
            break;
        case NotificationMessageV2::Subscribe:
            operationString = QLatin1String("Subscribe");
            break;
        case NotificationMessageV2::Unsubscribe:
            operationString = QLatin1String("Unsubscribe");
            break;
        default:
            operationString = QLatin1String("InvalidOp");
            break;
        };

        QStringList itemPartsList, addedFlagsList, removedFlagsList, addedTagsList, removedTagsList;
        foreach (const QByteArray &b, itemParts) {
            itemPartsList.push_back(QString::fromLatin1(b));
        }
        foreach (const QByteArray &b, addedFlags) {
            addedFlagsList.push_back(QString::fromLatin1(b));
        }
        foreach (const QByteArray &b, removedFlags) {
            removedFlagsList.push_back(QString::fromLatin1(b));
        }
        foreach (qint64 id, addedTags) {
            addedTagsList.push_back(QString::number(id));
        }
        foreach (qint64 id, removedTags) {
            removedTagsList.push_back(QString::number(id)) ;
        }

        const QString entry = QString::fromLatin1("session=%1 type=%2 operation=%3 items=%4 resource=%5 destResource=%6 parentCollection=%7 parentDestCollection=%8 itemParts=%9 addedFlags=%10 removedFlags=%11 addedTags=%12 removedTags=%13")
                              .arg(QString::fromLatin1(sessionId))
                              .arg(typeString)
                              .arg(operationString)
                              .arg(QVariant(items).toString())
                              .arg(QString::fromLatin1(resource))
                              .arg(QString::fromLatin1(destResource))
                              .arg(parentCollection)
                              .arg(parentDestCollection)
                              .arg(itemPartsList.join(QLatin1String(", ")))
                              .arg(addedFlagsList.join(QLatin1String(", ")))
                              .arg(removedFlagsList.join(QLatin1String(", ")))
                              .arg(addedTagsList.join(QLatin1String(", ")))
                              .arg(removedTagsList.join(QLatin1String(", ")));

        result += entry + QLatin1Char('\n');
    }

    return result;
}

void ChangeRecorderPrivate::addToStream(QDataStream &stream, const NotificationMessageV3 &msg)
{
    stream << msg.sessionId();
    stream << int(msg.type());
    stream << int(msg.operation());
    stream << msg.entities().count();
    Q_FOREACH (const NotificationMessageV2::Entity &entity, msg.entities()) {
        stream << quint64(entity.id);
        stream << entity.remoteId;
        stream << entity.remoteRevision;
        stream << entity.mimeType;
    }
    stream << msg.resource();
    stream << msg.destinationResource();
    stream << quint64(msg.parentCollection());
    stream << quint64(msg.parentDestCollection());
    stream << msg.itemParts();
    stream << msg.addedFlags();
    stream << msg.removedFlags();
    stream << msg.addedTags();
    stream << msg.removedTags();
}

void ChangeRecorderPrivate::writeStartOffset()
{
    if (!settings) {
        return;
    }

    QFile file(notificationsFileName());
    if (!file.open(QIODevice::ReadWrite)) {
        qWarning() << "Could not update notifications in file" << file.fileName();
        return;
    }

    // Skip "countAndVersion"
    file.seek(8);

    //kDebug() << "Writing start offset=" << m_startOffset;

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
        qWarning() << "Could not save notifications to file" << file.fileName();
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

    //kDebug() << "Saving" << pendingNotifications.count() << "notifications (full save)";

    for (int i = 0; i < pendingNotifications.count(); ++i) {
        const NotificationMessageV3 msg = pendingNotifications.at(i);
        addToStream(stream, msg);
    }
}

void ChangeRecorderPrivate::notificationsEnqueued(int count)
{
    // Just to ensure the contract is kept, and these two methods are always properly called.
    if (enableChangeRecording) {
        m_lastKnownNotificationsCount += count;
        if (m_lastKnownNotificationsCount != pendingNotifications.count()) {
            kWarning() << this << "The number of pending notifications changed without telling us! Expected"
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
