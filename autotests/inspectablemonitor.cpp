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

InspectableMonitorPrivate::InspectableMonitorPrivate(FakeMonitorDependeciesFactory *dependenciesFactory, InspectableMonitor* parent)
  : Akonadi::MonitorPrivate(dependenciesFactory, parent)
{
}

void InspectableMonitor::doConnectToNotificationManager()
{
  d_ptr->connectToNotificationManager();
}

InspectableMonitor::InspectableMonitor(FakeMonitorDependeciesFactory *dependenciesFactory, QObject *parent)
  : Monitor(new InspectableMonitorPrivate(dependenciesFactory, this), parent)
{
  // Make sure signals don't get optimized away.
  // TODO: Make this parametrizable in the test class.
  connect(this, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemRemoved(Akonadi::Item)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionChanged(Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionRemoved(Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionSubscribed(Akonadi::Collection,Akonadi::Collection)), SIGNAL(dummySignal()));
  connect(this, SIGNAL(collectionUnsubscribed(Akonadi::Collection)), SIGNAL(dummySignal()));

  QTimer::singleShot(0, this, SLOT(doConnectToNotificationManager()));
}

