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

#ifndef AKONADI_EVENT_QUEUE_H
#define AKONADI_EVENT_QUEUE_H

#include <QQueue>
#include <QMutex>

#include "monitor.h"
#include "session.h"
#include "collection.h"

using namespace Akonadi;

class AkonadiEvent : public QObject
{
  Q_OBJECT
public:

  explicit AkonadiEvent(QObject *parent = 0) : QObject(parent) {}

  void setAffectedCollections( QList<Entity::Id> ids );
  void setAffectedItems( QList<Entity::Id> ids );

  virtual bool isMonitorEvent() { return false; }
  virtual bool isSessionEvent() { return false; }
  virtual bool isTerminalEvent() { return false; }

  QList<Entity::Id> affectedCollections() const;
  QList<Entity::Id> affectedItems() const;

private:
  QList<Entity::Id> m_collectionIds;
  QList<Entity::Id> m_itemIds;
};

class MonitorEvent : public AkonadiEvent
{
  Q_OBJECT
public:
  explicit MonitorEvent(QObject *parent = 0) : AkonadiEvent(parent) {}

  enum EventType
  {
    CollectionsAdded,
    CollectionsRemoved,
    CollectionsChanged,
    CollectionsMoved,
    CollectionMonitored,
    CollecionUnMonitored,
    CollectionStatisticsChanged,
    ItemsAdded,
    ItemsRemoved,
    ItemsChanged,
    ItemsMoved,
    ItemLinked,
    ItemUnlinked,
    MimetypeMonitored,
    MimetypeUnMonitored,
    ResourceMonitored,
    ResourceUnMonitored
  };

  virtual bool isMonitorEvent() { return true; }

  void setEventType( EventType type ) { m_eventType = type; }
  EventType eventType() const { return m_eventType; }

private:
  EventType m_eventType;
};

class SessionEvent : public AkonadiEvent
{
  Q_OBJECT
public:
  explicit SessionEvent(QObject *parent = 0) : AkonadiEvent(parent), m_hasFollowUp(false) {}

  void setTrigger(const QByteArray &trigger) { m_trigger = trigger; }
  QByteArray trigger() const { return m_trigger; }

  void setResponse(const QByteArray &response) { m_response = response; }
  QByteArray response() const { return m_response; }

  void setHasFollowUpResponse(bool hasFollowup) { m_hasFollowUp = hasFollowup; }
  bool hasFollowUpResponse() const { return m_hasFollowUp; }

  void setHasTrigger(bool hasTrigger) { m_hasTrigger = hasTrigger; }
  bool hasTrigger() const { return m_hasTrigger; }

  virtual bool isSessionEvent() { return true; }

private:
  QByteArray m_trigger;
  QByteArray m_response;
  bool m_hasFollowUp;
  bool m_hasTrigger;
};

class TerminalEvent : public AkonadiEvent
{
  Q_OBJECT
public:
  explicit TerminalEvent(QObject *parent = 0) : AkonadiEvent(parent) {}

  virtual bool isTerminalEvent() { return true; }
};

/**
  Container for AkonadiEvents.

  Emits a dequeued signal each time an item is dequeued, so that th FakeAkonadiServer and the FakeSession
  can both respond as necessary.
*/
class EventQueue : public QObject
{
  Q_OBJECT
public:
  EventQueue( QObject *parent = 0 );

  bool isEmpty() const;

  void enqueue( AkonadiEvent* );
  AkonadiEvent* dequeue();
  AkonadiEvent* head();

signals:
  void dequeued();

private:
  QQueue<AkonadiEvent *> m_queue;
  QMutex m_mutex;

};

#endif
