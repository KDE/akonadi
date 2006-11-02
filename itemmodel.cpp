/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "itemquery.h"
#include "itemfetchjob.h"
#include "itemmodel.h"
#include "monitor.h"

#include <kmime/kmime_message.h>

#include <kdebug.h>
#include <klocale.h>

#include <QDebug>

using namespace Akonadi;

class ItemModel::Private
{
  public:
    QList<Item*> items;
    QString path;
    ItemFetchJob *listingJob;
    Monitor *monitor;
    QList<ItemQuery*> fetchJobs, updateJobs;
};

Akonadi::ItemModel::ItemModel( QObject *parent ) :
    QAbstractTableModel( parent ),
    d( new Private() )
{
  d->listingJob = 0;
  d->monitor = 0;
}

Akonadi::ItemModel::~ItemModel( )
{
  delete d->listingJob;
  delete d->monitor;
  qDeleteAll( d->items );
  qDeleteAll( d->fetchJobs );
  qDeleteAll( d->updateJobs );
  delete d;
}

QVariant Akonadi::ItemModel::data( const QModelIndex & index, int role ) const
{
  Q_UNUSED( role );
  if ( !index.isValid() )
    return QVariant();
  if ( index.row() >= d->items.count() )
    return QVariant();
  Item* itm = d->items.at( index.row() );
  return itm->data();
}

int Akonadi::ItemModel::rowCount( const QModelIndex & parent ) const
{
  if ( !parent.isValid() )
    return d->items.count();
  return 0;
}

QVariant Akonadi::ItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  return QAbstractTableModel::headerData( section, orientation, role );
}

void Akonadi::ItemModel::setPath( const QString& path )
{
  qWarning() << "ItemModel::setPath()";
  if ( d->path == path )
    return;
  d->path = path;
  // the query changed, thus everything we have already is invalid
  d->items.clear();
  reset();
  // stop all running jobs
  delete d->monitor;
  d->monitor = 0;
  qDeleteAll( d->updateJobs );
  d->updateJobs.clear();
  qDeleteAll( d->fetchJobs );
  d->updateJobs.clear();
  delete d->listingJob;
  // start listing job
  d->listingJob = createFetchJob( path, this );
  connect( d->listingJob, SIGNAL( done( Akonadi::Job* ) ), SLOT( listingDone( Akonadi::Job* ) ) );
  d->listingJob->start();
}

void Akonadi::ItemModel::listingDone( Akonadi::Job * job )
{
  Q_ASSERT( job == d->listingJob );
  if ( job->error() ) {
    // TODO
    kWarning() << k_funcinfo << "Item query failed!" << endl;
  } else {
    d->items = d->listingJob->items();
    reset();
  }
  d->listingJob->deleteLater();
  d->listingJob = 0;

  // start monitor job
  // TODO error handling
  /*d->monitor = new Monitor( "folder=" + d->path );
  connect( d->monitor, SIGNAL( changed( const DataReference::List& ) ),
           SLOT( messagesChanged( const DataReference::List& ) ) );
  connect( d->monitor, SIGNAL( added( const DataReference::List& ) ),
           SLOT( messagesAdded( const DataReference::List& ) ) );
  connect( d->monitor, SIGNAL( removed( const DataReference::List& ) ),
           SLOT( messagesRemoved( const DataReference::List& ) ) );*/
//   d->monitor->start();
}

void Akonadi::ItemModel::fetchingNewDone( Akonadi::Job * job )
{
  Q_ASSERT( d->fetchJobs.contains( static_cast<ItemQuery*>( job ) ) );
  if ( job->error() ) {
    // TODO
    kWarning() << k_funcinfo << "Fetching new items failed!" << endl;
  } else {
    Item::List list = static_cast<ItemQuery*>( job )->items();
    beginInsertRows( QModelIndex(), d->items.size(), d->items.size() + list.size() );
    d->items += list;
    endInsertRows();
  }
  d->fetchJobs.removeAll( static_cast<ItemQuery*>( job ) );
  job->deleteLater();
}

void Akonadi::ItemModel::fetchingUpdatesDone( Akonadi::Job * job )
{
  Q_ASSERT( d->updateJobs.contains( static_cast<ItemQuery*>( job ) ) );
  if ( job->error() ) {
    // TODO
    kWarning() << k_funcinfo << "Updating changed items failed!" << endl;
  } else {
    Item::List list = static_cast<ItemQuery*>( job )->items();
    foreach ( Item* itm, list ) {
      // ### *slow*
      for ( int i = 0; i < d->items.size(); ++i ) {
        if ( d->items.at( i )->reference() == itm->reference() ) {
          delete d->items.at( i );
          d->items.replace( i, itm );
          emit dataChanged( index( i, 0 ), index( i, columnCount() ) );
          break;
        }
      }
    }
  }
  d->updateJobs.removeAll( static_cast<ItemQuery*>( job ) );
  job->deleteLater();
}

void Akonadi::ItemModel::itemsChanged( const DataReference::List & references )
{
  // TODO: build query based on the reference list
  Q_UNUSED( references );
  QString query;
  ItemQuery* job = new ItemQuery( query );
  connect( job, SIGNAL( done( Akonadi::Job* ) ), SLOT( fetchingUpdatesDone( Akonadi::Job* job ) ) );
  job->start();
  d->updateJobs.append( job );
}

void Akonadi::ItemModel::itemsAdded( const DataReference::List & references )
{
  // TODO: build query based on the reference list
  Q_UNUSED( references );
  QString query;
  ItemQuery* job = new ItemQuery( query );
  connect( job, SIGNAL( done( Akonadi::Job* ) ), SLOT( fetchingNewDone( Akonadi::Job* job ) ) );
  job->start();
  d->fetchJobs.append( job );
}

void Akonadi::ItemModel::itemsRemoved( const DataReference::List & references )
{
  foreach ( DataReference ref, references ) {
    // ### *slow*
    int index = -1;
    for ( int i = 0; i < d->items.size(); ++i ) {
      if ( d->items.at( i )->reference() == ref ) {
        index = i;
        break;
      }
    }
    if ( index < 0 )
      continue;
    beginRemoveRows( QModelIndex(), index, index );
    Item* itm = d->items.at( index );
    d->items.removeAt( index );
    delete itm;
    endRemoveRows();
  }
}

DataReference Akonadi::ItemModel::referenceForIndex( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return DataReference();
  if ( index.row() >= d->items.count() )
    return DataReference();
  Item *itm = d->items.at( index.row() );
  Q_ASSERT( itm );
  return itm->reference();
}

Akonadi::Item* Akonadi::ItemModel::itemForIndex( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return 0;
  if ( index.row() >= d->items.count() )
    return 0;
  Item *itm = d->items.at( index.row() );
  Q_ASSERT( itm );
  return itm;
}

#include "itemmodel.moc"
