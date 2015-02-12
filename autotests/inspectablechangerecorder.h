/*
    Copyright (c) 2011 Stephen Kelly <steveire@gmail.com>

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

#ifndef INSPECTABLECHANGERECORDER_H
#define INSPECTABLECHANGERECORDER_H

#include "entitycache_p.h"
#include "changerecorder.h"
#include "changerecorder_p.h"

#include "fakeakonadiservercommand.h"
#include "fakeentitycache.h"
#include "akonaditestfake_export.h"

class InspectableChangeRecorder;

class InspectableChangeRecorderPrivate : public Akonadi::ChangeRecorderPrivate
{
public:
    InspectableChangeRecorderPrivate(FakeMonitorDependeciesFactory *dependenciesFactory, InspectableChangeRecorder *parent);
    ~InspectableChangeRecorderPrivate()
    {
    }

    bool emitNotification(const Akonadi::NotificationMessageV3 &msg) Q_DECL_OVERRIDE
    {
        // TODO: Check/Log
        return Akonadi::ChangeRecorderPrivate::emitNotification(msg);
    }
};

class AKONADITESTFAKE_EXPORT InspectableChangeRecorder : public Akonadi::ChangeRecorder
{
    Q_OBJECT
public:
    InspectableChangeRecorder(FakeMonitorDependeciesFactory *dependenciesFactory, QObject *parent = Q_NULLPTR);

    FakeNotificationSource *notifier() const {
        return qobject_cast<FakeNotificationSource *>(d_ptr->notificationSource->source());
    }

    QQueue<Akonadi::NotificationMessageV3> pendingNotifications() const
    {
        return d_ptr->pendingNotifications;
    }
    QQueue<Akonadi::NotificationMessageV3> pipeline() const
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
            Emitted
        };
        Position position;
    };
    QQueue<MessageStruct> m_messages;
};

#endif
