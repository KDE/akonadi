/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "inspectablemonitor.h"

void InspectableMonitor::doConnectToNotificationManager()
{
    d_ptr->connectToNotificationManager();
}

InspectableMonitor::InspectableMonitor(QObject *parent)
    : Monitor(new InspectableMonitorPrivate(this), parent)
{
    // Make sure signals don't get optimized away.
    // TODO: Make this parametrizable in the test class.
    connect(this, &Akonadi::Monitor::itemAdded, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::itemChanged, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::itemLinked, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::itemMoved, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::itemRemoved, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::itemUnlinked, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::collectionAdded, this, &InspectableMonitor::dummySignal);
    connect(this, SIGNAL(collectionChanged(Akonadi::Collection)), SIGNAL(dummySignal()));
    connect(this, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)), SIGNAL(dummySignal()));
    connect(this, &Akonadi::Monitor::collectionMoved, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::collectionRemoved, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::collectionStatisticsChanged, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::collectionSubscribed, this, &InspectableMonitor::dummySignal);
    connect(this, &Akonadi::Monitor::collectionUnsubscribed, this, &InspectableMonitor::dummySignal);

    QTimer::singleShot(0, this, [this]() { doConnectToNotificationManager(); });
}

