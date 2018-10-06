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
    QSettings *settings;
    bool enableChangeRecording;

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
    void writeStartOffset();

    int m_lastKnownNotificationsCount; // just for invariant checking
    int m_startOffset; // number of saved notifications to skip
    bool m_needFullSave;
};

} // namespace Akonadi

#endif
