/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "monitor.h"

class QSettings;

namespace Akonadi
{
class ChangeRecorderPrivate;

/**
 * @short Records and replays change notification.
 *
 * This class is responsible for recording change notifications while
 * an agent is not online and replaying the notifications when the agent
 * is online again. Therefore the agent doesn't have to care about
 * online/offline mode in its synchronization algorithm.
 *
 * Unlike Akonadi::Monitor this class only emits one change signal at a
 * time. To receive the next one you need to explicitly call replayNext().
 * If a signal is emitted that has no receivers, it's automatically skipped,
 * which means you only need to connect to signals you are actually interested
 * in.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT ChangeRecorder : public Monitor
{
    Q_OBJECT
public:
    /**
     * Creates a new change recorder.
     */
    explicit ChangeRecorder(QObject *parent = nullptr);

    /**
     * Destroys the change recorder.
     * All not yet processed changes are written back to the config file.
     */
    ~ChangeRecorder();

    /**
     * Sets the QSettings object used for persistent recorded changes.
     */
    void setConfig(QSettings *settings);

    /**
     * Returns whether there are recorded changes.
     */
    Q_REQUIRED_RESULT bool isEmpty() const;

    /**
     * Removes the previously emitted change from the records.
     */
    void changeProcessed();

    /**
     * Enables change recording. If change recording is disabled, this class
     * behaves exactly like Akonadi::Monitor.
     * Change recording is enabled by default.
     * @param enable @c false to disable change recording. @c true by default
     */
    void setChangeRecordingEnabled(bool enable);

    /**
     * Debugging: dump current list of notifications, as saved on disk.
     */
    Q_REQUIRED_RESULT QString dumpNotificationListToString() const;

public Q_SLOTS:
    /**
     * Replay the next change notification and erase the previous one from the record.
     */
    void replayNext();

Q_SIGNALS:
    /**
     * Emitted when new changes are recorded.
     */
    void changesAdded();

    /**
     * Emitted when replayNext() was called, but there was no valid change to replay.
     * This can happen when all pending changes have been filtered out, for example.
     * You only need to connect to this signal if you rely on one signal being emitted
     * as a result of calling replayNext().
     */
    void nothingToReplay();

protected:
    /// @cond PRIVATE
    explicit ChangeRecorder(ChangeRecorderPrivate *d, QObject *parent = nullptr);
    /// @endcond

private:
    /// @cond PRIVATE
    Q_DECLARE_PRIVATE(ChangeRecorder)
    /// @endcond
};

}

