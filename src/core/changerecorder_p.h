/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_CHANGERECORDER_P_H
#define AKONADI_CHANGERECORDER_P_H

#include "akonadiprivate_export.h"
#include "changerecorder.h"
#include "monitor_p.h"

class QDataStream;

namespace Akonadi
{

class ChangeRecorder;
class ChangeNotificationDependenciesFactory;

class AKONADI_TESTS_EXPORT ChangeRecorderPrivate : public Akonadi::MonitorPrivate
{
public:
    ChangeRecorderPrivate(ChangeNotificationDependenciesFactory *dependenciesFactory_, ChangeRecorder *parent);

    Q_DECLARE_PUBLIC(ChangeRecorder)
    QSettings *settings = nullptr;
    bool enableChangeRecording = true;

    int pipelineSize() const override;
    void notificationsEnqueued(int count) override;
    void notificationsErased() override;

    void slotNotify(const Protocol::ChangeNotificationPtr &msg) override;
    bool emitNotification(const Protocol::ChangeNotificationPtr &msg) override;

    QString notificationsFileName() const;

    void loadNotifications();
    QString dumpNotificationListToString() const;
    void addToStream(QDataStream &stream, const Protocol::ChangeNotificationPtr &msg);
    void saveNotifications();
private:
    void dequeueNotification();
    void notificationsLoaded();
    void writeStartOffset() const;

    int m_lastKnownNotificationsCount = 0; // just for invariant checking
    int m_startOffset = 0; // number of saved notifications to skip
    bool m_needFullSave = true;
};

} // namespace Akonadi

#endif
