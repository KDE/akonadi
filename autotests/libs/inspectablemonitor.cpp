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

#include "inspectablemonitor.h"

InspectableMonitorPrivate::InspectableMonitorPrivate(FakeMonitorDependenciesFactory *dependenciesFactory, InspectableMonitor *parent)
    : Akonadi::MonitorPrivate(dependenciesFactory, parent)
{
}

void InspectableMonitor::doConnectToNotificationManager()
{
    d_ptr->connectToNotificationManager();
}

InspectableMonitor::InspectableMonitor(FakeMonitorDependenciesFactory *dependenciesFactory, QObject *parent)
    : Monitor(new InspectableMonitorPrivate(dependenciesFactory, this), parent)
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

