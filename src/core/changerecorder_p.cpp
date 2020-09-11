/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changerecorder_p.h"
#include "akonadicore_debug.h"
#include "changerecorderjournal_p.h"


#include <QFile>
#include <QDir>
#include <QSettings>
#include <QFileInfo>
#include <QDataStream>

using namespace Akonadi;

ChangeRecorderPrivate::ChangeRecorderPrivate(ChangeNotificationDependenciesFactory *dependenciesFactory_,
        ChangeRecorder *parent)
    : MonitorPrivate(dependenciesFactory_, parent)
{
}

int ChangeRecorderPrivate::pipelineSize() const
{
    if (enableChangeRecording) {
        return 0; // we fill the pipeline ourselves when using change recording
    }
    return MonitorPrivate::pipelineSize();
}

void ChangeRecorderPrivate::slotNotify(const Protocol::ChangeNotificationPtr &msg)
{
    Q_Q(ChangeRecorder);
    const int oldChanges = pendingNotifications.size();
    // with change recording disabled this will automatically take care of dispatching notification messages and saving
    MonitorPrivate::slotNotify(msg);
    if (enableChangeRecording && pendingNotifications.size() != oldChanges) {
        Q_EMIT q->changesAdded();
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
        settings->beginGroup(QStringLiteral("ChangeRecorder"));
        const int size = settings->beginReadArray(QStringLiteral("change"));

        for (int i = 0; i < size; ++i) {
            settings->setArrayIndex(i);
            auto msg = ChangeRecorderJournalReader::loadQSettingsNotification(settings);
            if (msg->isValid()) {
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
        pendingNotifications = ChangeRecorderJournalReader::loadFrom(&file, m_needFullSave);
    } else {
        m_needFullSave = true;
    }
    notificationsLoaded();
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
    const auto notifications = ChangeRecorderJournalReader::loadFrom(&file, dummy);
    for (const auto &n : notifications) {
        result += Protocol::debugString(n) + QLatin1Char('\n');
    }
    return result;
}

void ChangeRecorderPrivate::writeStartOffset() const
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
    ChangeRecorderJournalWriter::saveTo(pendingNotifications, &file);
    m_needFullSave = false;
    m_startOffset = 0;
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

bool ChangeRecorderPrivate::emitNotification(const Protocol::ChangeNotificationPtr &msg)
{
    const bool someoneWasListening = MonitorPrivate::emitNotification(msg);
    if (!someoneWasListening && enableChangeRecording) {
        //If no signal was emitted (e.g. because no one was connected to it), no one is going to call changeProcessed, so we help ourselves.
        dequeueNotification();
        QMetaObject::invokeMethod(q_ptr, "replayNext", Qt::QueuedConnection);
    }
    return someoneWasListening;
}
