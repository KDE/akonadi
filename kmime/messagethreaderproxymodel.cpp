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

  /*
   * Reset everything
   */
  void slotCollectionChanged()
  {
    childrenMap.clear();
    indexMap.clear();
    parentMap.clear();
    realPerfectParentsMap.clear();
    realUnperfectParentsMap.clear();
    realSubjectParentsMap.clear();

    realPerfectChildrenMap.clear();
    realUnperfectChildrenMap.clear();
    realSubjectChildrenMap.clear();

    mParent->reset();
  }

  /*
   * Function called when the signal rowsInserted was triggered in the
   * source model.
   */
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
      // Get his best potential parent using the mail threader parts
      readParentsFromParts( item );
      int parentId = parentForItem( item.reference().id() );

      /*
       * Fill in the tree maps
       */
      int row = childrenMap[ parentId ].count();
      mParent->beginInsertRows( indexMap[ parentId ], row, row );
      childrenMap[ parentId ] << item.reference().id();
      parentMap[ id ] = parentId;
      QModelIndex index = mParent->createIndex( childrenMap[ parentId ].count() - 1, 0, id );
      mParent->endInsertRows();


      /*
       * Look for potential children into real children map
       */
      QList<int> potentialChildren = realPerfectChildrenMap[ id ]
                                     << realUnperfectChildrenMap[ id ]
                                     << realSubjectChildrenMap[ id ];
      foreach( int potentialChildId, potentialChildren ) {
          // This item can be a child of our item if:
          // - it's not the item itself (could we do that check when building the 'real' maps ?)
          // - his parent is set
          // -   and this parent is not already our item
          if ( potentialChildId != id &&
               parentMap.constFind( potentialChildId ) != parentMap.constEnd() &&
               parentMap[ potentialChildId ] != id &&
               parentMap[ potentialChildId ]
            )

          {
            // Check that the current parent of this item is not better than ours
            QList<int> realParentsList = realPerfectParentsMap[ potentialChildId ]
                                         << realUnperfectParentsMap[ potentialChildId ]
                                         << realSubjectParentsMap[ potentialChildId ];
            int currentParentPos = realParentsList.indexOf( parentMap[ potentialChildId ] );
            // currentParentPos = 0 is probably the more common case so we may avoid an indexOf.
            if ( currentParentPos == 0 || ( currentParentPos != -1 && realParentsList.indexOf( id ) > currentParentPos ) )
              // (currentParentPos can be -1 if parent is root)
              continue;

            // Remove the children from the old location
            int childRow = childrenMap[ parentMap[ potentialChildId ] ].indexOf( potentialChildId );
            mParent->beginRemoveRows( indexMap[ parentMap[ potentialChildId ] ], childRow, childRow );
            mParent->endRemoveRows();
            childrenMap[ parentMap[ potentialChildId ] ].removeAt( childRow );

            // Change the tree info
            mParent->beginInsertRows( index, childrenMap[ id ].count(), childrenMap[ id ].count() );
            parentMap[ potentialChildId ] = id;
            childrenMap[ id ] << potentialChildId;

            // Recreate index because row change
            mParent->createIndex( childrenMap[ id ].count() - 1, 0, potentialChildId );
            mParent->endInsertRows();
          }
      }
    }

    qDebug() << time.elapsed() / 1000 << " seconds for " << end - begin + 1 << " items";
  }

  /*
   * Function called when the signal rowsAboutToBeRemoved is sent by the source model
   * (source model indexes are *still* valid)
   */
  void slotRemoveRows( const QModelIndex& sourceIndex, int begin, int end )
  {
    Q_UNUSED( sourceIndex );
    for ( int i = begin; i <= end; i++ )
    {
      Item item = sourceMessageModel()->itemForIndex( sourceMessageModel()->index( i, 0 ) );
      int id = item.reference().id();
      int parentId = parentMap[ id ];
      int row = childrenMap[ parentId ].indexOf( id );

      // Reparent the children to the closest parent
      foreach( int childId, childrenMap[ id ] ) {
        int childRow = childrenMap[ id ].indexOf( childId );
        mParent->beginRemoveRows( indexMap[ id ], childRow, childRow );
        childrenMap[ id ].removeAll( childId ); // There is only one ...
        mParent->endRemoveRows();

        mParent->beginInsertRows( indexMap[ parentId ], childrenMap[ parentId ].count(),
                                  childrenMap[ parentId ].count() );
        parentMap[ childId ] = parentId;
        childrenMap[ parentId ] << childId;
        mParent->endInsertRows();

        mParent->createIndex( childrenMap[ parentId ].count() - 1, 0, childId ); // Is it necessary to recreate the index ?
      }

      mParent->beginRemoveRows( indexMap[ parentId ], row, row );
      childrenMap[ parentId ].removeAll( id ); // Remove this id from the children of parentId
      parentMap.remove( id );
      indexMap.remove( id );
      mParent->endRemoveRows();
//      mParent->beginRemoveColumns( indexMap[ parentId ], 0, sourceMessageModel()->columnCount() - 1 );
//      mParent->endRemoveColumns();
    }
  }

  /*
   * This item has his parents stored in his threading parts.
   * Read them and store them in the 'real' maps.
   *
   * We store both relationships :
   * - child -> parents ( real*ParentsMap )
   * - parent -> children ( real*ChildrenMap )
   */
  void readParentsFromParts( const Item& item )
  {
    QList<int> realPerfectParentsList = readParentsFromPart( item, MailThreaderAgent::PartPerfectParents );
    QList<int> realUnperfectParentsList = readParentsFromPart( item, MailThreaderAgent::PartUnperfectParents );
    QList<int> realSubjectParentsList = readParentsFromPart( item, MailThreaderAgent::PartSubjectParents );

    realPerfectParentsMap[ item.reference().id() ] = realPerfectParentsList;
    realUnperfectParentsMap[ item.reference().id() ] = realUnperfectParentsList;
    realSubjectParentsMap[ item.reference().id() ] = realSubjectParentsList;

    // Fill in the children maps
    foreach( int parentId, realPerfectParentsList )
      realPerfectChildrenMap[ parentId ] << item.reference().id();
    foreach( int parentId, realUnperfectParentsList )
      realUnperfectChildrenMap[ parentId ] << item.reference().id();
    foreach( int parentId, realSubjectParentsList )
      realSubjectChildrenMap[ parentId ] << item.reference().id();
  }

  // Helper function for the precedent one
  QList<int> readParentsFromPart( const Item& item, const QLatin1String& part  )
  {
    bool ok = false;
    QList<QByteArray> parentsIds = item.part( part ).split( ',' );
    QList<int> result;
    int parentId;
    foreach( QByteArray s, parentsIds ) {
      parentId = s.toInt( &ok );
      if( !ok )
        continue;
      result << parentId;
    }

    return result;
  }

  /*
   * Find the first parent in the parents maps which is actually in the current collection
   * @param id the item id
   * @returns the parent id
   */
  int parentForItem( int id )
  {

    QList<int> parentsIds;
    parentsIds << realPerfectParentsMap[ id ] << realUnperfectParentsMap[ id ] << realSubjectParentsMap[ id ];

    foreach( int parentId, parentsIds )
    {
    // Check that the parent is in the collection
    // This is time consuming but ... required.
    if ( sourceMessageModel()->indexForItem( DataReference( parentId, QString() ), 0 ).isValid() )
      return parentId;

    }

    // TODO Check somewhere for 'parent loops' : in the parts, an item child of his child ...
    return -1;
  }

  // -1 is an invalid id which means 'root'
  int idForIndex( const QModelIndex& index )
  {
    return index.isValid() ? index.internalId() : -1;
  }

  MessageThreaderProxyModel *mParent;

  /*
   * These maps store the current tree structure, as presented in the view.
   * It tries to be as close as possible from the real structure, given that not every parents
   * are present in the collection
   */
  QMap<int, QList<int> > childrenMap;
  QMap<int, int> parentMap;
  QMap<int, QModelIndex> indexMap;

  /*
   * These maps store the real parents, as read from the item parts
   * In the best case, the list should contain only one element ( = unique parent )
   * If there isn't only one, the algorithm will pick up the first one in the current collection
   */
  QMap<int, QList<int> > realPerfectParentsMap;
  QMap<int, QList<int> > realUnperfectParentsMap;
  QMap<int, QList<int> > realSubjectParentsMap;

  QMap<int, QList<int> > realPerfectChildrenMap;
  QMap<int, QList<int> > realUnperfectChildrenMap;
  QMap<int, QList<int> > realSubjectChildrenMap;
};

MessageThreaderProxyModel::MessageThreaderProxyModel( QObject *parent )
  : QAbstractProxyModel( parent ),
    d( new Private( this ) )
{
  setSupportedDragActions( Qt::MoveAction | Qt::CopyAction );
}

MessageThreaderProxyModel::~MessageThreaderProxyModel()
{
  delete d;
}


// ### Awful ! Already in mailthreaderagent.cpp ; just for testing purposes, though
const QLatin1String MailThreaderAgent::PartPerfectParents = QLatin1String( "AkonadiMailThreaderAgentPerfectParents" );
const QLatin1String MailThreaderAgent::PartUnperfectParents = QLatin1String( "AkonadiMailThreaderAgentUnperfectParents");
const QLatin1String MailThreaderAgent::PartSubjectParents = QLatin1String( "AkonadiMailThreaderAgentSubjectParents" );

QModelIndex MessageThreaderProxyModel::index( int row, int column, const QModelIndex& parent ) const
{
  int parentId = d->idForIndex( parent );

  if ( row < 0
       || column < 0
       || row >= d->childrenMap[ parentId ].count()
       || column >= columnCount( parent )
       )
    return QModelIndex();

  int id = d->childrenMap[ parentId ].at( row );

  return createIndex( row, column, id );
}

QModelIndex MessageThreaderProxyModel::parent( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return QModelIndex();

  int parentId = d->parentMap[ index.internalId() ];

  if ( parentId == -1 )
    return QModelIndex();

//  int parentParentId = d->parentMap[ parentId ];
  //int row = d->childrenMap[ parentParentId ].indexOf( parentId );
  return d->indexMap[ d->parentMap[ index.internalId() ] ];
    //return createIndex( row, 0, parentId );
}

QModelIndex MessageThreaderProxyModel::mapToSource( const QModelIndex& index ) const
{
  // This function is slow because it relies on rowForItem in the ItemModel (linear time)
  return d->sourceMessageModel()->indexForItem( DataReference( index.internalId(), QString() ), index.column() );
}

QModelIndex MessageThreaderProxyModel::mapFromSource( const QModelIndex& index ) const
{
  Item item = d->sourceMessageModel()->itemForIndex( index );
  int id = item.reference().id();
  //return d->indexMap[ id  ]; // FIXME take column in account like mapToSource
  return MessageThreaderProxyModel::index( d->indexMap[ id ].row(), index.column(), d->indexMap[ id ].parent() );
}

QModelIndex MessageThreaderProxyModel::createIndex( int row, int column, quint32 internalId ) const
{
  QModelIndex index = QAbstractProxyModel::createIndex( row, column, internalId );
  if ( column == 0 )
    d->indexMap[ internalId ] = index; // Store the newly created index in the index map
  return index;
}

void MessageThreaderProxyModel::setSourceModel( QAbstractItemModel* model )
{
  // TODO Assert model is a MessageModel
  QAbstractProxyModel::setSourceModel( model );

  d->sourceMessageModel()->addFetchPart( MailThreaderAgent::PartPerfectParents );
  d->sourceMessageModel()->addFetchPart( MailThreaderAgent::PartUnperfectParents );
  d->sourceMessageModel()->addFetchPart( MailThreaderAgent::PartSubjectParents );

  // TODO disconnect old model
  connect( sourceModel(), SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( slotInsertRows( QModelIndex, int, int ) ) );
  connect( sourceModel(), SIGNAL( rowsAboutToBeRemoved( QModelIndex, int, int ) ), SLOT( slotRemoveRows( QModelIndex, int, int ) ) );
  connect( d->sourceMessageModel(), SIGNAL( collectionSet( Collection ) ), SLOT( slotCollectionChanged() ) );
}


bool MessageThreaderProxyModel::hasChildren( const QModelIndex& index ) const
{
  return rowCount( index ) > 0;
}

int MessageThreaderProxyModel::columnCount( const QModelIndex& index ) const
{
  // We assume that the source model has the same number of columns for each rows
  return sourceModel()->columnCount( QModelIndex() );
}

int MessageThreaderProxyModel::rowCount( const QModelIndex& index ) const
{
  int id = d->idForIndex( index );
  if ( id == -1 )
    return d->childrenMap[ -1 ].count();

  if ( index.column() == 0 ) // QModelIndex() has children
    return d->childrenMap[ id ].count();

  return 0;
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
