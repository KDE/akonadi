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

#include "partfetcher.h"

#include "entitytreemodel.h"
#include "session.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"

using namespace Akonadi;

namespace Akonadi
{

class PartFetcherPrivate
{
  PartFetcherPrivate( PartFetcher *partFetcher )
      : q_ptr( partFetcher )
  {

  }

  void itemsFetched( const Akonadi::Item::List &list );
  void fetchJobDone( KJob *job );

  void modelDataChanged( const QModelIndex &topLeft, const QModelIndex &bottomRight );

  QPersistentModelIndex m_persistentIndex;
  QByteArray m_partName;

  Q_DECLARE_PUBLIC( PartFetcher )
  PartFetcher *q_ptr;

};

}

void PartFetcherPrivate::itemsFetched( const Akonadi::Item::List &list )
{
  Q_Q(PartFetcher);

  Q_ASSERT( list.size() == 1 );

  // If m_persistentIndex comes from a selection proxy model, it could become
  // invalid if the user clicks around a lot.
  if( !m_persistentIndex.isValid() )
  {
    emit q->invalidated();
    q->reset();
    return;
  }

  QSet<QByteArray> loadedParts = m_persistentIndex.data( EntityTreeModel::LoadedPartsRole ).value<QSet<QByteArray> >();

  Q_ASSERT( !loadedParts.contains( m_partName ) );

  Item item = m_persistentIndex.data( EntityTreeModel::ItemRole ).value<Item>();

  item.merge( list.at( 0 ) );

  QAbstractItemModel *model = const_cast<QAbstractItemModel *>( m_persistentIndex.model() );

  Q_ASSERT( model );

  QVariant itemVariant = QVariant::fromValue( item );
  model->setData( m_persistentIndex, itemVariant, EntityTreeModel::ItemRole );

  emit q->partFetched( m_persistentIndex, item, m_partName );
}

void PartFetcherPrivate::fetchJobDone( KJob *job )
{
  Q_Q( PartFetcher );
  if ( job->error() )
  {
    emit q->invalidated();
  }
  q->reset();
}

PartFetcher::PartFetcher( QObject *parent )
  : QObject( parent ), d_ptr( new PartFetcherPrivate( this ) )
{

}

bool PartFetcher::fetchPart( const QModelIndex &idx, const QByteArray &partName )
{
  Q_D( PartFetcher );

  Q_ASSERT( idx.isValid() );

  if ( d->m_persistentIndex.isValid() || !d->m_partName.isEmpty() )
    // One PartFetcher can only handle one fetch operation at a time.
    return false;

  QSet<QByteArray> loadedParts = idx.data( EntityTreeModel::LoadedPartsRole ).value<QSet<QByteArray> >();

  if ( loadedParts.contains( partName ) )
  {
    Item item = idx.data( EntityTreeModel::ItemRole ).value<Item>();
    emit partFetched( idx, item, partName );
    reset();
    return true;
  }

  QSet<QByteArray> availableParts = idx.data( EntityTreeModel::AvailablePartsRole ).value<QSet<QByteArray> >();

  if ( !availableParts.contains( partName ) )
  {
    return false;
  }

  Akonadi::Session *session = qobject_cast<Akonadi::Session *>( qvariant_cast<QObject *>( idx.data( EntityTreeModel::SessionRole ) ) );

  if (!session)
    return false;

  Akonadi::Item item = idx.data( EntityTreeModel::ItemRole ).value<Akonadi::Item>();

  if (!item.isValid())
    return false;

  d->m_persistentIndex = idx;
  d->m_partName = partName;

  ItemFetchScope scope;
  scope.fetchPayloadPart( partName );
  ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob( item, session );
  itemFetchJob->setFetchScope( scope );

  connect( itemFetchJob, SIGNAL( itemsReceived( const Akonadi::Item::List& ) ),
              this, SLOT( itemsFetched( const Akonadi::Item::List& ) ) );
  connect( itemFetchJob, SIGNAL( result( KJob* ) ),
              this, SLOT( fetchJobDone( KJob* ) ) );

  return true;

}

void PartFetcher::reset()
{
  Q_D( PartFetcher );

  d->m_persistentIndex = QModelIndex();
  d->m_partName.clear();
}

#include "partfetcher.moc"
