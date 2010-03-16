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

#ifndef FAKEMONITOR_H
#define FAKEMONITOR_H

#include "changerecorder.h"

namespace Akonadi
{
class Collection;
class Item;
}

class EventQueue;
class FakeAkonadiServer;

using namespace Akonadi;

class FakeMonitor : public Akonadi::ChangeRecorder
{
  Q_OBJECT
public:
  FakeMonitor(QObject* parent = 0);

signals:
  void emit_collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent  );
  void emit_collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &target );
  void emit_collectionRemoved( const Akonadi::Collection &collection );

  void emit_itemAdded( const Akonadi::Item &item, const Akonadi::Collection &parent  );
  void emit_itemMoved( const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &target );
  void emit_itemRemoved( const Akonadi::Item &item );

  void emit_itemLinked( const Akonadi::Item &item, const Akonadi::Collection &collection );
  void emit_itemUnlinked( const Akonadi::Item &item, const Akonadi::Collection &collection );


public slots:
  void processNextEvent();

private:
  void connectForwardingSignals();

private:
  EventQueue *m_eventQueue;
  FakeAkonadiServer *m_fakeServer;

};

#endif

