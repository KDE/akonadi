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

#include "fakeserverdata.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/collectionfetchjob.h>

FakeServerData::FakeServerData( EntityTreeModel *model, FakeSession *session, FakeMonitor *monitor, QObject *parent )
  : QObject(parent),
    m_model( model ),
    m_session( session ),
    m_monitor( monitor ),
    m_nextCollectionId( 1 ),
    m_nextItemId( 0 )
{
  connect(session, SIGNAL(jobAdded(Akonadi::Job*)), SLOT(jobAdded(Akonadi::Job*)), Qt::QueuedConnection);
}

void FakeServerData::setCommands(QList< FakeAkonadiServerCommand* > list)
{
  m_communicationQueue.clear();
  foreach( FakeAkonadiServerCommand *command, list )
    m_communicationQueue << command;
}

void FakeServerData::processNotifications()
{
  while ( !m_communicationQueue.isEmpty() )
  {
    FakeAkonadiServerCommand::Type respondTo = m_communicationQueue.head()->respondTo();
    if ( respondTo == FakeAkonadiServerCommand::Notification )
    {
      FakeAkonadiServerCommand *command = m_communicationQueue.dequeue();
      command->doCommand();
    } else {
      return;
    }
  }
}

void FakeServerData::jobAdded( Job* job )
{
  Entity::Id fetchColId = job->property( "FetchCollectionId" ).toULongLong();

  returnEntities( fetchColId );
}

void FakeServerData::returnEntities( Entity::Id fetchColId )
{
  if ( !returnCollections( fetchColId ) )
    while ( !m_communicationQueue.isEmpty() && m_communicationQueue.head()->respondTo() == FakeAkonadiServerCommand::RespondToItemFetch )
      returnItems( fetchColId );

  processNotifications();
}

bool FakeServerData::returnCollections( Entity::Id fetchColId )
{
  if ( m_communicationQueue.isEmpty() )
    return true;
  FakeAkonadiServerCommand::Type commType = m_communicationQueue.head()->respondTo();

  Collection fetchCollection = m_communicationQueue.head()->fetchCollection();

  if ( commType == FakeAkonadiServerCommand::RespondToCollectionFetch
       && fetchColId == fetchCollection.id() )
  {
    FakeAkonadiServerCommand *command = m_communicationQueue.dequeue();
    command->doCommand();
    if ( !m_communicationQueue.isEmpty() )
      returnEntities( fetchColId );
    return true;
  }
  return false;
}

void FakeServerData::returnItems( Entity::Id fetchColId )
{
  FakeAkonadiServerCommand::Type commType = m_communicationQueue.head()->respondTo();

  if ( commType == FakeAkonadiServerCommand::RespondToItemFetch )
  {
    FakeAkonadiServerCommand *command = m_communicationQueue.dequeue();
    command->doCommand();
    if ( !m_communicationQueue.isEmpty() )
      returnEntities( fetchColId );
  }
}

