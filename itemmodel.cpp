/*
    Copyright (c) 2006 - 2007 Volker Krause <volker.krause@rwth-aachen.de>

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
    QList<ItemFetchJob*> fetchJobs;
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
  delete d;
}

QVariant Akonadi::ItemModel::data( const QModelIndex & index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();
  if ( index.row() >= d->items.count() )
    return QVariant();
  Item* item = d->items.at( index.row() );
  if ( !item )
    return QVariant();

  if ( role == Qt::DisplayRole ) {
    switch ( index.column() ) {
      case Id:
        return QString::number( item->reference().persistanceID() );
      case MimeType:
        return QString::fromLatin1( item->mimeType() );
    }
  }

  return QVariant();
}

int Akonadi::ItemModel::rowCount( const QModelIndex & parent ) const
{
  if ( !parent.isValid() )
    return d->items.count();
  return 0;
}

int ItemModel::columnCount(const QModelIndex & parent) const
{
  if ( !parent.isValid() )
    return 2; // keep in sync with Column enum
  return 0;
}

QVariant Akonadi::ItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    switch ( section ) {
      case Id:
        return i18n( "Id" );
      case MimeType:
        return i18n( "MimeType" );
      default:
        return QString();
    }
  }
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
  qDeleteAll( d->fetchJobs );
  d->fetchJobs.clear();
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

  // start monitor
  d->monitor = new Monitor( this );
  d->monitor->monitorCollection( d->path, false );
  connect( d->monitor, SIGNAL(itemChanged(Akonadi::DataReference)),
           SLOT(itemChanged(Akonadi::DataReference)) );
  connect( d->monitor, SIGNAL(itemAdded(Akonadi::DataReference)),
           SLOT(itemAdded(Akonadi::DataReference)) );
  connect( d->monitor, SIGNAL(itemRemoved(Akonadi::DataReference)),
           SLOT(itemRemoved(Akonadi::DataReference)) );
}

void Akonadi::ItemModel::fetchingNewDone( Akonadi::Job * job )
{
  Q_ASSERT( d->fetchJobs.contains( static_cast<ItemFetchJob*>( job ) ) );
  if ( job->error() ) {
    // TODO
    kWarning() << k_funcinfo << "Fetching new items failed!" << endl;
  } else {
    Item::List list = static_cast<ItemFetchJob*>( job )->items();
    if ( !list.isEmpty() ) {
      beginInsertRows( QModelIndex(), d->items.size(), d->items.size() + list.size() );
      d->items += list;
      endInsertRows();
    } else
      kWarning() << k_funcinfo << "Got unexpected empty fetch response!" << endl;
  }
  d->fetchJobs.removeAll( static_cast<ItemFetchJob*>( job ) );
  job->deleteLater();
}

void ItemModel::itemChanged( const DataReference &reference )
{
  itemRemoved( reference );
  itemAdded( reference );
}

void ItemModel::itemAdded( const DataReference &reference )
{
  // TODO: make sure we don't fetch the complete data here!
  ItemFetchJob *job = new ItemFetchJob( reference, this );
  connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(fetchingNewDone(Akonadi::Job*)) );
  job->start();
  d->fetchJobs.append( job );
}

void ItemModel::itemRemoved( const DataReference &reference )
{
  // ### *slow*
  int index = -1;
  for ( int i = 0; i < d->items.size(); ++i ) {
    if ( d->items.at( i )->reference() == reference ) {
      index = i;
      break;
    }
  }
  if ( index < 0 )
    return;
  beginRemoveRows( QModelIndex(), index, index );
  Item* itm = d->items.at( index );
  d->items.removeAt( index );
  delete itm;
  endRemoveRows();
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

ItemFetchJob* ItemModel::createFetchJob(const QString & path, QObject * parent)
{
  return new ItemFetchJob( path, parent );
}

#include "itemmodel.moc"
