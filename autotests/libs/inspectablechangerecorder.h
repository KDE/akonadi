/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "changerecorder.h"
#include "changerecorder_p.h"
#include "entitycache_p.h"

#include "akonaditestfake_export.h"
#include "fakeakonadiservercommand.h"
#include "fakeentitycache.h"

class InspectableChangeRecorder;

class InspectableChangeRecorderPrivate : public Akonadi::ChangeRecorderPrivate
{
public:
    InspectableChangeRecorderPrivate(FakeMonitorDependenciesFactory *dependenciesFactory, InspectableChangeRecorder *parent);
    ~InspectableChangeRecorderPrivate() override
    {
    }

    bool emitNotification(const Akonadi::Protocol::ChangeNotificationPtr &msg) override
    {
        // TODO: Check/Log
        return Akonadi::ChangeRecorderPrivate::emitNotification(msg);
    }
};

class AKONADITESTFAKE_EXPORT InspectableChangeRecorder : public Akonadi::ChangeRecorder
{
    Q_OBJECT
public:
    explicit InspectableChangeRecorder(FakeMonitorDependenciesFactory *dependenciesFactory, QObject *parent = nullptr);

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
