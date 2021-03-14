/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef INSPECTABLEMONITOR_H
#define INSPECTABLEMONITOR_H

#include "entitycache_p.h"
#include "monitor.h"
#include "monitor_p.h"

#include "akonaditestfake_export.h"
#include "fakeakonadiservercommand.h"
#include "fakeentitycache.h"

class InspectableMonitor;

class InspectableMonitorPrivate : public Akonadi::MonitorPrivate
{
public:
    InspectableMonitorPrivate(FakeMonitorDependenciesFactory *dependenciesFactory, InspectableMonitor *parent);
    ~InspectableMonitorPrivate() override
    {
    }

    bool emitNotification(const Akonadi::Protocol::ChangeNotificationPtr &msg) override
    {
        // TODO: Check/Log
        return Akonadi::MonitorPrivate::emitNotification(msg);
    }
};

class AKONADITESTFAKE_EXPORT InspectableMonitor : public Akonadi::Monitor
{
    Q_OBJECT
public:
    explicit InspectableMonitor(FakeMonitorDependenciesFactory *dependenciesFactory, QObject *parent = nullptr);

    FakeNotificationConnection *notificationConnection() const
    {
        return qobject_cast<FakeNotificationConnection *>(d_ptr->ntfConnection);
    }

    QQueue<Akonadi::Protocol::ChangeNotificationPtr> pendingNotifications() const
    {
        return d_ptr->pendingNotifications;
    }
    QQueue<Akonadi::Protocol::ChangeNotificationPtr> pipeline() const
    {
        return d_ptr->pipeline;
    }

Q_SIGNALS:
    void dummySignal();

private Q_SLOTS:
    void dispatchNotifications()
    {
        d_ptr->dispatchNotifications();
    }

    void doConnectToNotificationManager();

private:
    struct MessageStruct {
        enum Position {
            Queued,
            FilterPipelined,
            Pipelined,
            Emitted,
        };
        Position position;
    };
    QQueue<MessageStruct> m_messages;
};

#endif
