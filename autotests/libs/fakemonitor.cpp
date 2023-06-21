/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fakemonitor.h"
#include "changerecorder_p.h"

#include "entitycache_p.h"

#include <QMetaMethod>

using namespace Akonadi;

class FakeMonitorPrivate : public ChangeRecorderPrivate
{
    Q_DECLARE_PUBLIC(FakeMonitor)
public:
    explicit FakeMonitorPrivate(FakeMonitor *monitor)
        : ChangeRecorderPrivate(nullptr, monitor)
    {
    }

    bool connectToNotificationManager() override
    {
        // Do nothing. This monitor should not connect to the notification manager.
        return true;
    }
};

FakeMonitor::FakeMonitor(QObject *parent)
    : ChangeRecorder(new FakeMonitorPrivate(this), parent)
{
}

#include "moc_fakemonitor.cpp"
