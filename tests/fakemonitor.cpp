/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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


#include "fakemonitor.h"

#include "akonadieventqueue.h"
#include "fakeserver.h"

using namespace Akonadi;

FakeMonitor::FakeMonitor(EventQueue *eventQueue, FakeAkonadiServer *fakeServer, QObject* parent)
  : Monitor(parent), m_eventQueue(eventQueue), m_fakeServer(fakeServer)
{
  connect(eventQueue, SIGNAL(dequeued()), SLOT(processNextEvent()));
}


void FakeMonitor::processNextEvent()
{
  if (!m_eventQueue->head()->isMonitorEvent())
    return;

  AkonadiEvent *event = m_eventQueue->dequeue();
  MonitorEvent *monitorEvent = dynamic_cast<MonitorEvent *>(event);
  Q_ASSERT(monitorEvent);

  QList<Entity::Id> ids = monitorEvent->affectedCollections();



  // Process it.
  switch (monitorEvent->eventType())
  {
    case MonitorEvent::CollectionsAdded:
    {
      Collection::List list;
      foreach (Entity::Id id, ids)
      {
        Collection col = m_fakeServer->getCollection(id);
        Q_ASSERT(col.isValid());
        Collection parent = m_fakeServer->getCollection(col.parentCollection().id());
        Q_ASSERT(parent.isValid());
        emit collectionAdded(col, parent);
      }
    }
  }
}




