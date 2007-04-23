/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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
#include "session.h"

#include <kmime/kmime_message.h>

#include <kdebug.h>
#include <klocale.h>

#include <QtCore/QDebug>

using namespace Akonadi;

class ItemModel::Private
{
  public:
    Private( ItemModel *parent )
      : mParent( parent ), monitor( 0 )
    {
      session = new Session( QByteArray("ItemModel-") + QByteArray::number( qrand() ), mParent );
    }

    ~Private()
    {
      delete monitor;
    }

    void listingDone( KJob* );
    void itemChanged( const Akonadi::Item&, const QStringList& );
    void itemAdded( const Akonadi::Item& );
    void itemRemoved( const Akonadi::DataReference& );

    ItemModel *mParent;
    Item::List items;
    Collection collection;
    Monitor *monitor;
    Session *session;
};

void ItemModel::Private::listingDone( KJob * job )
{
  ItemFetchJob *fetch = static_cast<ItemFetchJob*>( job );
  if ( job->error() ) {
    // TODO
    kWarning() << k_funcinfo << "Item query failed!" << endl;
  } else {
    items = fetch->items();
    mParent->reset();
  }

  // start monitor
  monitor = new Monitor( mParent );
  monitor->addFetchPart( ItemFetchJob::PartAll );
  monitor->ignoreSession( session );
  monitor->monitorCollection( collection );
  mParent->connect( monitor, SIGNAL(itemChanged( const Akonadi::Item&, const QStringList& )),
                    mParent, SLOT(itemChanged( const Akonadi::Item&, const QStringList& )) );
  mParent->connect( monitor, SIGNAL(itemAdded( const Akonadi::Item&, const Akonadi::Collection& )),
                    mParent, SLOT(itemAdded( const Akonadi::Item& )) );
  mParent->connect( monitor, SIGNAL(itemRemoved(Akonadi::DataReference)),
                    mParent, SLOT(itemRemoved(Akonadi::DataReference)) );
}

void ItemModel::Private::itemChanged( const Akonadi::Item &item, const QStringList& )
{
  itemRemoved( item.reference() );
  itemAdded( item );
}

void ItemModel::Private::itemAdded( const Akonadi::Item &item )
{
  mParent->beginInsertRows( QModelIndex(), items.size(), items.size() + 1 );
  items.append( item );
  mParent->endInsertRows();
}

void ItemModel::Private::itemRemoved( const DataReference &reference )
{
  // ### *slow*
  int index = -1;
  for ( int i = 0; i < items.size(); ++i ) {
    if ( items.at( i ).reference() == reference ) {
      index = i;
      break;
    }
  }
  if ( index < 0 )
    return;

  mParent->beginRemoveRows( QModelIndex(), index, index );
  const Item item = items.at( index );
  Q_ASSERT( item.isValid() );
  items.removeAt( index );
  mParent->endRemoveRows();
}

ItemModel::ItemModel( QObject *parent ) :
    QAbstractTableModel( parent ),
    d( new Private( this ) )
{
}

ItemModel::~ItemModel()
{
  delete d;
}

QVariant ItemModel::data( const QModelIndex & index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();
  if ( index.row() >= d->items.count() )
    return QVariant();
  const Item item = d->items.at( index.row() );
  if ( !item.isValid() )
    return QVariant();

  if ( role == Qt::DisplayRole ) {
    switch ( index.column() ) {
      case Id:
        return QString::number( item.reference().id() );
      case RemoteId:
        return item.reference().remoteId();
      case MimeType:
        return item.mimeType();
    }
  }

  return QVariant();
}

int ItemModel::rowCount( const QModelIndex & parent ) const
{
  if ( !parent.isValid() )
    return d->items.count();
  return 0;
}

int ItemModel::columnCount(const QModelIndex & parent) const
{
  if ( !parent.isValid() )
    return 3; // keep in sync with Column enum
  return 0;
}

QVariant ItemModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    switch ( section ) {
      case Id:
        return i18n( "Id" );
      case RemoteId:
        return i18n( "Remote Id" );
      case MimeType:
        return i18n( "MimeType" );
      default:
        return QString();
    }
  }
  return QAbstractTableModel::headerData( section, orientation, role );
}

void ItemModel::setCollection( const Collection &collection )
{
  qWarning() << "ItemModel::setPath()";
  if ( d->collection == collection )
    return;
  d->collection = collection;
  // the query changed, thus everything we have already is invalid
  d->items.clear();
  reset();
  // stop all running jobs
  d->session->clear();
  delete d->monitor;
  d->monitor = 0;
  // start listing job
  ItemFetchJob* job = new ItemFetchJob( collection, session() );
  job->addFetchPart( ItemFetchJob::PartAll );
  connect( job, SIGNAL(result(KJob*)), SLOT(listingDone(KJob*)) );
}

DataReference ItemModel::referenceForIndex( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return DataReference();
  if ( index.row() >= d->items.count() )
    return DataReference();

  const Item item = d->items.at( index.row() );
  Q_ASSERT( item.isValid() );
  return item.reference();
}

Item ItemModel::itemForIndex( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return Akonadi::Item();

  if ( index.row() >= d->items.count() )
    return Akonadi::Item();

  Item item = d->items.at( index.row() );
  Q_ASSERT( item.isValid() );

  return item;
}

Session * ItemModel::session() const
{
  return d->session;
}

#include "itemmodel.moc"
