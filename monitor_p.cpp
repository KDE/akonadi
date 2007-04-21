/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include "collectionlistjob.h"
#include "itemfetchjob.h"

#include "monitor_p.h"

using namespace Akonadi;

ItemCollectionFetchJob::ItemCollectionFetchJob( const DataReference &reference, int collectionId, QObject *parent )
  : Job( parent ),
    mReference( reference ), mCollectionId( collectionId )
{
}

ItemCollectionFetchJob::~ItemCollectionFetchJob()
{
}

Item ItemCollectionFetchJob::item() const
{
  return mItem;
}

Collection ItemCollectionFetchJob::collection() const
{
  return mCollection;
}

void ItemCollectionFetchJob::doStart()
{
  CollectionListJob *listJob = new CollectionListJob( Collection( mCollectionId ), CollectionListJob::Local, this );
  connect( listJob, SIGNAL( result( KJob* ) ), SLOT( collectionJobDone( KJob* ) ) );
  addSubjob( listJob );

  ItemFetchJob *fetchJob = new ItemFetchJob( mReference, this );
  connect( fetchJob, SIGNAL( result( KJob* ) ), SLOT( itemJobDone( KJob* ) ) );
  addSubjob( fetchJob );
}

void ItemCollectionFetchJob::collectionJobDone( KJob* job )
{
  if ( !job->error() ) {
    CollectionListJob *listJob = qobject_cast<CollectionListJob*>( job );
    if ( listJob->collections().count() == 0 ) {
      setError( 1 );
      setErrorText( QLatin1String( "No collection found" ) );
    } else
      mCollection = listJob->collections().first();
  }
}

void ItemCollectionFetchJob::itemJobDone( KJob* job )
{
  if ( !job->error() ) {
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );
    if ( fetchJob->items().count() == 0 ) {
      setError( 2 );
      setErrorText( QLatin1String( "No item found" ) );
    } else
      mItem = fetchJob->items().first();

    emitResult();
  }
}

#include "monitor_p.moc"
