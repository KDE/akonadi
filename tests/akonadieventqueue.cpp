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


#include "akonadieventqueue.h"

#include <QTimer>

void AkonadiEvent::setAffectedCollections(QList< Entity::Id > ids)
{
  m_collectionIds = ids;
}

void AkonadiEvent::setAffectedItems(QList< Entity::Id > ids)
{
  m_itemIds = ids;
}

QList< Entity::Id > AkonadiEvent::affectedCollections() const
{
  return m_collectionIds;
}

QList< Entity::Id > AkonadiEvent::affectedItems() const
{
  return m_itemIds;
}

EventQueue::EventQueue(QObject* parent)
    : QObject(parent)
{

}

AkonadiEvent* EventQueue::dequeue()
{
  QMutexLocker locker(&m_mutex);

  AkonadiEvent* akonadiEvent = m_queue.dequeue();
  QTimer::singleShot( 0, this, SIGNAL( dequeued() ) );
  return akonadiEvent;
}

void EventQueue::enqueue(AkonadiEvent* akonadiEvent )
{
  m_queue.enqueue( akonadiEvent );
}

bool EventQueue::isEmpty() const
{
  return m_queue.isEmpty();
}

AkonadiEvent* EventQueue::head()
{
  QMutexLocker locker(&m_mutex);

  if (m_queue.isEmpty())
    return new AkonadiEvent();

  return m_queue.head();
}


#include "akonadieventqueue.moc"



