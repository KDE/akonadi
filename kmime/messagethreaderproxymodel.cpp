/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#include "messagethreaderproxymodel.h"
#include <libakonadi/itemfetchjob.h>
#include <agents/mailthreader/mailthreaderagent.h>
#include <messagemodel.h>
#include <libakonadi/datareference.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QTime>
#include <QtCore/QModelIndex>

using namespace Akonadi;

class MessageThreaderProxyModel::Private
{
  public:
    Private( MessageThreaderProxyModel *parent )
      : mParent( parent )
    {
    }


  MessageModel* sourceMessageModel()
  {
    return dynamic_cast<MessageModel*>( mParent->sourceModel() );
  }

  void slotCollectionChanged()
  {
    qDebug() << "collection changed";
    childrenMap.clear();
    indexMap.clear();
    parentMap.clear();

    mParent->reset();
  }

  void slotInsertRows( const QModelIndex& sourceIndex, int begin, int end )
  {
    Q_UNUSED( sourceIndex ); // parent source index is always invalid (flat source model)
    QTime time;
    time.start();

    for ( int i=begin; i <= end; i++ )
    {
      // Retrieve the item from the source model
      Item item = sourceMessageModel()->itemForIndex( sourceMessageModel()->index( i, 0 ) );
      int id = item.reference().id();

      // Get his parent using the PartParent
      bool ok = false;
      int parentId = item.part( MailThreaderAgent::PartParent ).toInt( &ok );
      if( !ok )
        parentId = -1;

      // Fill in the tree

      // Check that the parent is in the collection
      // This is time consuming but ... required.
      if ( !sourceMessageModel()->indexForItem( DataReference( parentId, QString() ), 0 ).isValid() )
        parentId = -1;

      childrenMap[ parentId ] << item.reference().id();
      parentMap[ id ] = parentId;

      // ### This is not correct, indexMap[ parentId ] does not exist
      // ### Handle insertion of parents who arrive after the children (does it work ?)
      mParent->beginInsertRows( indexMap[ parentId ], childrenMap[ parentId ].count() - 1, childrenMap[ parentId ].count() - 1 );
      mParent->endInsertRows();
      mParent->beginInsertColumns( indexMap[ parentId ], 0, sourceMessageModel()->columnCount() - 1 );
      mParent->endInsertColumns();
    }

    qDebug() << time.elapsed() / 1000 << " seconds for " << end - begin + 1 << " items";
  }


  // ### Untested
  void slotRemoveRows( const QModelIndex& sourceIndex, int begin, int end )
  {
    // Who is the parent ?
    QModelIndex parentIndex = mParent->mapFromSource( sourceIndex );
    int parentId = parentIndex.internalId();

    for ( int i=begin; i <= end; i++ )
    {
      Item item = sourceMessageModel()->itemForIndex( sourceMessageModel()->index( i, 0 ) );
      int id = item.reference().id();

      int row = indexMap[ id ].row();
      childrenMap[ parentId ].removeAt( row ); // Remove this id from the children of parentId
      parentMap.remove( id );
      indexMap.remove( id );
      mParent->beginRemoveRows( parentIndex, row, row );
      mParent->endRemoveRows();
      mParent->beginInsertColumns( parentIndex, 0, sourceMessageModel()->columnCount() - 1 );
      mParent->endInsertColumns();

    }
  }

  MessageThreaderProxyModel *mParent;
  QMap<int, QList<int> > childrenMap;
  QMap<int, int> parentMap;
  QMap<int, QModelIndex> indexMap;
};

MessageThreaderProxyModel::MessageThreaderProxyModel( QObject *parent )
  : QAbstractProxyModel( parent ),
    d( new Private( this ) )
{

}

MessageThreaderProxyModel::~MessageThreaderProxyModel()
{
  delete d;
}


// ### Awful ! Already in mailthreaderagent.cpp ; just for testing purposes, though
const QLatin1String MailThreaderAgent::PartParent = QLatin1String( "AkonadiMailThreaderAgentParent" );
const QLatin1String MailThreaderAgent::PartSort = QLatin1String( "AkonadiMailThreaderAgentSort");

QModelIndex MessageThreaderProxyModel::index( int row, int column, const QModelIndex& parent ) const
{
  int parentId = parent.isValid() ? parent.internalId() : -1;

  if ( row >= d->childrenMap[ parentId ].count() )
    return QModelIndex();

  int id = d->childrenMap[ parentId ].at( row );
  qDebug() << "messagethreaderproxymodel, id = " << id << " and parent = " << parentId;
  return createIndex( row, column, id );
}

QModelIndex MessageThreaderProxyModel::parent ( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return QModelIndex();

  int parentId = d->parentMap[ index.internalId() ];
  if ( parentId == -1 )
    return QModelIndex();

  int parentParentId = d->parentMap[ parentId ];
  int row = d->childrenMap[ parentParentId ].indexOf( parentId );
  return createIndex( row, 0, parentId );
}

QModelIndex MessageThreaderProxyModel::mapToSource( const QModelIndex& index ) const
{
//  qDebug() << "call here";
  return d->sourceMessageModel()->indexForItem( DataReference( index.internalId(), QString() ), index.column() );
}

QModelIndex MessageThreaderProxyModel::mapFromSource( const QModelIndex& index ) const
{
  Item item = d->sourceMessageModel()->itemForIndex( index );
  int id = item.reference().id();
  return d->indexMap[ id  ]; // take column in account like mapToSource
}

QModelIndex MessageThreaderProxyModel::createIndex( int row, int column, quint32 internalId ) const
{
  QModelIndex index = QAbstractProxyModel::createIndex( row, column, internalId );
  d->indexMap[ internalId ] = index;
  return index;
}

void MessageThreaderProxyModel::setSourceModel( QAbstractItemModel* model )
{
  // TODO Assert model is a MessageModel
  QAbstractProxyModel::setSourceModel( model );

  d->sourceMessageModel()->addFetchPart( MailThreaderAgent::PartParent );

  // TODO disconnect old model

  connect( sourceModel(), SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( slotInsertRows( QModelIndex, int, int ) ) );
  connect( sourceModel(), SIGNAL( rowsAboutToBeRemoved( QModelIndex, int, int ) ), SLOT( slotRemoveRows( QModelIndex, int, int ) ) );
  connect( d->sourceMessageModel(), SIGNAL( collectionSet( Collection ) ), SLOT( slotCollectionChanged() ) );
}


bool MessageThreaderProxyModel::hasChildren( const QModelIndex& index ) const
{
  bool hasChildren = rowCount( index ) > 0;
  return hasChildren;
}

int MessageThreaderProxyModel::columnCount( const QModelIndex& index ) const
{
  return sourceModel()->columnCount( mapToSource( index ) );
}

int MessageThreaderProxyModel::rowCount( const QModelIndex& index ) const
{
  int count;

  if ( !index.isValid() ) // Root child count
    count = d->childrenMap[ -1 ].count();
  else // Normal item child count
    count = d->childrenMap[ d->sourceMessageModel()->itemForIndex( mapToSource( index ) ).reference().id() ].count();

  return count;
}

QStringList MessageThreaderProxyModel::mimeTypes() const
{
  return d->sourceMessageModel()->mimeTypes();
}

QMimeData *MessageThreaderProxyModel::mimeData(const QModelIndexList &indexes) const
{
    QModelIndexList sourceIndexes;
    for (int i = 0; i < indexes.count(); i++)
        sourceIndexes << mapToSource( indexes.at(i) );

    return sourceModel()->mimeData(sourceIndexes);
}

#include "messagethreaderproxymodel.moc"
