/*
  Copyright (C) 2009 Stephen Kelly <steveire@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef FAKE_SERVER_DATA_H
#define FAKE_SERVER_DATA_H

#include <QSharedPointer>
#include <QQueue>

#include "job.h"
#include "entitytreemodel.h"

#include "fakesession.h"
#include "fakemonitor.h"
#include "fakeakonadiservercommand.h"
#include "akonaditestfake_export.h"

using namespace Akonadi;

class AKONADITESTFAKE_EXPORT FakeServerData : public QObject
{
  Q_OBJECT
public:
  FakeServerData( EntityTreeModel *model, FakeSession *session, FakeMonitor *monitor, QObject *parent = 0 );

  void setCommands( QList<FakeAkonadiServerCommand*> list );

  Entity::Id nextCollectionId() const { return m_nextCollectionId++; }
  Entity::Id nextItemId()       const { return m_nextItemId++;       }

  Akonadi::EntityTreeModel* model() const { return m_model; }

  void processNotifications();

private Q_SLOTS:
  void jobAdded( Akonadi::Job *job );

private:
  bool returnCollections( Entity::Id fetchColId );
  void returnItems( Entity::Id fetchColId );
  void returnEntities( Entity::Id fetchColId );

private:
  EntityTreeModel *m_model;
  FakeSession *m_session;
  FakeMonitor *m_monitor;

  QList<FakeAkonadiServerCommand*> m_commandList;
  QQueue<FakeAkonadiServerCommand*> m_communicationQueue;

  mutable Entity::Id m_nextCollectionId;
  mutable Entity::Id m_nextItemId;
};

#endif
