/*
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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

#include "recursivemover_p.h"
#include "collectionfetchjob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "collectionfetchscope.h"

using namespace Akonadi;

RecursiveMover::RecursiveMover(AgentBasePrivate* parent): KCompositeJob(parent),
  m_agentBase( parent ), m_currentAction( None ), m_runningJobs( 0 ), m_pendingReplay( false )
{
}

void RecursiveMover::start()
{
  Q_ASSERT( receivers(SIGNAL(result(KJob*))) );

  CollectionFetchJob* job = new CollectionFetchJob( m_movedCollection, CollectionFetchJob::Recursive, this );
  connect( job, SIGNAL(finished(KJob*)), SLOT(collectionListResult(KJob*)) );
  addSubjob( job );
  ++m_runningJobs;
}

void RecursiveMover::setCollection( const Akonadi::Collection &collection )
{
  m_movedCollection = collection;
}

void RecursiveMover::collectionListResult(KJob* job)
{
  Q_ASSERT(m_pendingCollections.isEmpty());
  --m_runningJobs;

  if ( job->error() )
    return; // errror handling is in the base class

  // build a parent -> children map for the following topological sorting
  CollectionFetchJob* fetchJob = qobject_cast<CollectionFetchJob*>( job );
  QHash<Collection::Id, Collection::List> colTree;
  foreach ( const Collection &col, fetchJob->collections() ) {
    colTree[col.parentCollection().id()] << col;
  }

  // topological sort; BFS traversal of the tree
  m_pendingCollections.push_back( m_movedCollection );
  QQueue<Collection> toBeProcessed;
  toBeProcessed.enqueue( m_movedCollection );
  while ( !toBeProcessed.isEmpty() ) {
    const Collection col = toBeProcessed.dequeue();
    const Collection::List kids = colTree.value( col.id() );
    if ( kids.isEmpty() )
      continue;
    m_pendingCollections.append( kids );
    foreach ( const Collection &kid, kids )
      toBeProcessed.enqueue( kid );
  }

  replayNextCollection();
}

void RecursiveMover::collectionFetchResult(KJob *job)
{
  Q_ASSERT( m_currentCollection.isValid() );
  --m_runningJobs;

  if ( job->error() )
    return; // error handling is in the base class

  CollectionFetchJob* fetchJob = qobject_cast<CollectionFetchJob*>( job );
  if ( fetchJob->collections().size() == 1 ) {
    m_currentCollection = fetchJob->collections().first(); // TODO update inside tree
  } else {
    // already deleted, move on
  }

  if ( !m_runningJobs && m_pendingReplay )
    replayNext();
}

void RecursiveMover::itemListResult(KJob* job)
{
  --m_runningJobs;

  if ( job->error() )
    return; // errror handling is in the base class

  foreach ( const Item &item, qobject_cast<ItemFetchJob*>( job )->items() ) {
    if ( item.remoteId().isEmpty() )
      m_pendingItems.push_back( item );
  }

  if ( m_currentAction == None ) // we did not need to replay the collection, so continue with the items here
    replayNextItem();
  if ( !m_runningJobs && m_pendingReplay )
    replayNext();
}

void RecursiveMover::itemFetchResult(KJob* job)
{
  Q_ASSERT( m_currentAction == None );
  --m_runningJobs;

  if ( job->error() )
    return; // errror handling is in the base class

  ItemFetchJob* fetchJob = qobject_cast<ItemFetchJob*>( job );
  if ( fetchJob->items().size() == 1 ) {
    m_currentAction = AddItem;
    m_agentBase->itemAdded( fetchJob->items().first(), m_currentCollection );
  } else {
    // deleted since we started, skip
    m_currentItem = Item();
    replayNextItem();
  }
}

void RecursiveMover::replayNextCollection()
{
  if ( !m_pendingCollections.isEmpty() ) {

    m_currentCollection = m_pendingCollections.takeFirst();
    ItemFetchJob* job = new ItemFetchJob( m_currentCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(itemListResult(KJob*)));
    addSubjob( job );
    ++m_runningJobs;

    if ( m_currentCollection.remoteId().isEmpty() ) {
      Q_ASSERT( m_currentAction == None );
      m_currentAction = AddCollection;
      m_agentBase->collectionAdded( m_currentCollection, m_currentCollection.parentCollection() ); // TODO real full parents
      return;
    } else {
      //replayNextItem(); - but waiting for the fetch job to finish first
      return;
    }
  } else {
    // nothing left to do
    emitResult();
  }
}

void RecursiveMover::replayNextItem()
{
  Q_ASSERT( m_currentCollection.isValid() );
  if ( m_pendingItems.isEmpty() ) {
    replayNextCollection(); // all items processed here
    return;
  } else {
    Q_ASSERT( m_currentAction == None );
    m_currentItem = m_pendingItems.takeFirst();
    ItemFetchJob* job = new ItemFetchJob( m_currentItem, this );
    job->fetchScope().fetchFullPayload();
    connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchResult(KJob*)) );
    addSubjob( job );
    ++m_runningJobs;
  }
}

void RecursiveMover::changeProcessed()
{
  Q_ASSERT( m_currentAction != None );

  if ( m_currentAction == AddCollection ) {
    Q_ASSERT( m_currentCollection.isValid() );
    CollectionFetchJob *job = new CollectionFetchJob( m_currentCollection, CollectionFetchJob::Base, this );
    job->fetchScope().setAncestorRetrieval( CollectionFetchScope::All );
    connect( job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)) );
    addSubjob( job );
    ++m_runningJobs;
  }

  m_currentAction = None;
}

void RecursiveMover::replayNext()
{
  // wait for runnings jobs to finish before actually doing the replay
  if ( m_runningJobs ) {
    m_pendingReplay = true;
    return;
  }

  if ( m_currentCollection.isValid() )
    replayNextItem();
  else
    replayNextCollection();
}

#include "recursivemover_p.moc"
