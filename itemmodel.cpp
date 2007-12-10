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

#include "itemmodel.h"

#include "itemfetchjob.h"
#include "monitor.h"
#include "session.h"

#include <kmime/kmime_message.h>

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>

#include <QtCore/QDebug>
#include <QtCore/QMimeData>

using namespace Akonadi;

/*
 * This struct is used for optimization reasons.
 * because it embeds the row.
 *
 * Semantically, we could have used an item instead.
 */
struct ItemContainer
{
  ItemContainer( const Item& i, int r )
  {
    item = i;
    row = r;
  }
  Item item;
  int row;
};

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
    void itemsAdded( const Akonadi::Item::List &list );
    void itemAdded( const Akonadi::Item &item );
    void itemMoved( const Akonadi::Item&, const Akonadi::Collection& src, const Akonadi::Collection& dst );
    void itemRemoved( const Akonadi::DataReference& );
    int rowForItem( const Akonadi::DataReference& );

    ItemModel *mParent;

    QList<ItemContainer*> items;
    QHash<DataReference, ItemContainer*> itemHash;

    Collection collection;
    Monitor *monitor;
    Session *session;
    QStringList mFetchParts;
};

void ItemModel::Private::listingDone( KJob * job )
{
  ItemFetchJob *fetch = static_cast<ItemFetchJob*>( job );
  if ( job->error() ) {
    // TODO
    kWarning( 5250 ) << "Item query failed:" << job->errorString();
  }

  // start monitor
  monitor = new Monitor( mParent );
  foreach( QString part, mFetchParts )
    monitor->addFetchPart( part );

  monitor->ignoreSession( session );
  monitor->monitorCollection( collection );
  mParent->connect( monitor, SIGNAL(itemChanged( const Akonadi::Item&, const QStringList& )),
                    mParent, SLOT(itemChanged( const Akonadi::Item&, const QStringList& )) );
  mParent->connect( monitor, SIGNAL(itemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& )),
                    mParent, SLOT(itemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& ) ) );
  mParent->connect( monitor, SIGNAL(itemAdded( const Akonadi::Item&, const Akonadi::Collection& )),
                    mParent, SLOT(itemAdded( const Akonadi::Item& )) );
  mParent->connect( monitor, SIGNAL(itemRemoved(Akonadi::DataReference)),
                    mParent, SLOT(itemRemoved(Akonadi::DataReference)) );
}

int ItemModel::Private::rowForItem( const Akonadi::DataReference& ref )
{
  ItemContainer *container = itemHash.value( ref );
  if ( !container )
    return -1;

  /* Try to find the item directly;

     If items have been removed, this first try won't succeed because
     the ItemContainer rows have not been updated (costs too much).
  */
  if ( container->row < items.count()
       && items.at( container->row ) == container )
    return container->row;
  else { // Slow solution if the fist one has not succeeded
    int row = -1;
    for ( int i = 0; i < items.size(); ++i ) {
      if ( items.at( i )->item.reference() == ref ) {
        row = i;
        break;
      }
    }
    return row;
  }

}

void ItemModel::Private::itemChanged( const Akonadi::Item &item, const QStringList& )
{
  int row = rowForItem( item.reference() );
  if ( row < 0 )
    return;
  QModelIndex index = mParent->index( row, 0, QModelIndex() );

  mParent->dataChanged( index, index );
}

void ItemModel::Private::itemMoved( const Akonadi::Item &item, const Akonadi::Collection& colSrc, const Akonadi::Collection& colDst )
{
  if ( colSrc == collection && colDst != collection ) // item leaving this model
  {
    itemRemoved( item.reference() );
    return;
  }


  if ( colDst == collection && colSrc != collection )
  {
    itemAdded( item );
    return;
  }
}

void ItemModel::Private::itemsAdded( const Akonadi::Item::List &list )
{
  if ( list.isEmpty() )
    return;
  mParent->beginInsertRows( QModelIndex(), items.count(), items.count() + list.count() - 1 );
  foreach( Item item, list ) {
    ItemContainer *c = new ItemContainer( item, items.count() );
    items.append( c );
    itemHash[ item.reference() ] = c;
  }
  mParent->endInsertRows();
}

void ItemModel::Private::itemAdded( const Akonadi::Item &item )
{
  Item::List l;
  l << item;
  itemsAdded( l );
}

void ItemModel::Private::itemRemoved( const DataReference &reference )
{
  int row = rowForItem( reference );
  if ( row < 0 )
    return;

  mParent->beginRemoveRows( QModelIndex(), row, row );
  const Item item = items.at( row )->item;
  Q_ASSERT( item.isValid() );
  itemHash.remove( item.reference() );
  delete items.takeAt( row );
  mParent->endRemoveRows();
}

ItemModel::ItemModel( QObject *parent ) :
    QAbstractTableModel( parent ),
    d( new Private( this ) )
{
  setSupportedDragActions( Qt::MoveAction | Qt::CopyAction );
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
  const Item item = d->items.at( index.row() )->item;
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

  if ( role == IdRole ) {
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
  foreach( ItemContainer *c, d->items )
    delete c;
  d->items.clear();
  reset();

  // stop all running jobs
  d->session->clear();
  delete d->monitor;
  d->monitor = 0;

  // start listing job
  ItemFetchJob* job = new ItemFetchJob( collection, session() );
  foreach( QString part, d->mFetchParts )
    job->addFetchPart( part );
  connect( job, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(itemsAdded(Akonadi::Item::List)) );
  connect( job, SIGNAL(result(KJob*)), SLOT(listingDone(KJob*)) );

  emit collectionSet( collection );
}

void ItemModel::addFetchPart( const QString &identifier )
{
  if ( !d->mFetchParts.contains( identifier ) )
    d->mFetchParts.append( identifier );

  // update the monitor
  if ( d->monitor ) {
    foreach( QString part, d->mFetchParts )
      d->monitor->addFetchPart( part );
  }
}

DataReference ItemModel::referenceForIndex( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return DataReference();
  if ( index.row() >= d->items.count() )
    return DataReference();

  const Item item = d->items.at( index.row() )->item;
  Q_ASSERT( item.isValid() );
  return item.reference();
}

Item ItemModel::itemForIndex( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return Akonadi::Item();

  if ( index.row() >= d->items.count() )
    return Akonadi::Item();

  Item item = d->items.at( index.row() )->item;
  Q_ASSERT( item.isValid() );

  return item;
}

Qt::ItemFlags ItemModel::flags( const QModelIndex &index ) const
{
  Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

  if (index.isValid())
    return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
  else
    return Qt::ItemIsDropEnabled | defaultFlags;
}

QStringList ItemModel::mimeTypes() const
{
  return QStringList();
}

Session * ItemModel::session() const
{
  return d->session;
}

QMimeData *ItemModel::mimeData( const QModelIndexList &indexes ) const
{
  QMimeData *data = new QMimeData();
  // Add item uri to the mimedata for dropping in external applications
  KUrl::List urls;
  foreach ( QModelIndex index, indexes ) {
    if ( index.column() != 0 )
      continue;

    urls << itemForIndex( index ).url();
  }
  urls.populateMimeData( data );

  return data;
}

QModelIndex ItemModel::indexForItem( const Akonadi::DataReference& ref, const int column ) const
{
  return index( d->rowForItem( ref ), column );
}

#include "itemmodel.moc"
