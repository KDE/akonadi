/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changerecorder.h"
#include "changerecorder_p.h"

#include <QSettings>

using namespace Akonadi;

ChangeRecorder::ChangeRecorder(QObject *parent)
    : Monitor(new ChangeRecorderPrivate(nullptr, this), parent)
{
}

ChangeRecorder::ChangeRecorder(ChangeRecorderPrivate *privateclass, QObject *parent)
    : Monitor(privateclass, parent)
{
}

ChangeRecorder::~ChangeRecorder()
{
}

void ChangeRecorder::setConfig(QSettings *settings)
{
    Q_D(ChangeRecorder);
    if (settings) {
        d->settings = settings;
        Q_ASSERT(d->pendingNotifications.isEmpty());
        d->loadNotifications();
    } else if (d->settings) {
        if (d->enableChangeRecording) {
            d->saveNotifications();
        }
        d->settings = settings;
    }
}

void ChangeRecorder::replayNext()
{
    Q_D(ChangeRecorder);

    if (!d->enableChangeRecording) {
        return;
    }

    if (!d->pendingNotifications.isEmpty()) {
        const auto msg = d->pendingNotifications.head();
        if (d->ensureDataAvailable(msg)) {
            d->emitNotification(msg);
        } else if (d->translateAndCompress(d->pipeline, msg)) {
            // The msg is now in both pipeline and pendingNotifications.
            // When data is available, MonitorPrivate::flushPipeline will emitNotification.
            // When changeProcessed is called, we'll finally remove it from pendingNotifications.
        } else {
            // In the case of a move where both source and destination are
            // ignored, we ignore the message and process the next one.
            d->dequeueNotification();
            replayNext();
            return;
        }
    } else {
        // This is necessary when none of the notifications were accepted / processed
        // above, and so there is no one to call changeProcessed() and the ChangeReplay task
        // will be stuck forever in the ResourceScheduler.
        Q_EMIT nothingToReplay();
    }
}

bool ChangeRecorder::isEmpty() const
{
    Q_D(const ChangeRecorder);
    return d->pendingNotifications.isEmpty();
}

void ChangeRecorder::changeProcessed()
{
    Q_D(ChangeRecorder);

    if (!d->enableChangeRecording) {
        return;
    }

    // changerecordertest.cpp calls changeProcessed after receiving nothingToReplay,
    // so test for emptiness. Not sure real code does this though.
    // Q_ASSERT( !d->pendingNotifications.isEmpty() )
    if (!d->pendingNotifications.isEmpty()) {
        d->dequeueNotification();
    }
}

void ChangeRecorder::setChangeRecordingEnabled(bool enable)
{
    Q_D(ChangeRecorder);
    if (d->enableChangeRecording == enable) {
        return;
    }
    d->enableChangeRecording = enable;
    if (enable) {
        d->m_needFullSave = true;
        d->notificationsLoaded();
    } else {
        d->dispatchNotifications();
    }
}

QString Akonadi::ChangeRecorder::dumpNotificationListToString() const
{
    Q_D(const ChangeRecorder);
    return d->dumpNotificationListToString();
}

#include "moc_changerecorder.cpp"
