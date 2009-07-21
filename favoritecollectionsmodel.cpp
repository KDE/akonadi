/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>


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

#include "favoritecollectionsmodel.h"

#include <QtGui/QItemSelectionModel>

#include <kconfiggroup.h>
#include <kglobal.h>
#include <ksharedconfig.h>

#include "entitytreemodel.h"

using namespace Akonadi;

/**
 * @internal
 */
class FavoriteCollectionsModel::Private
{
  public:
    Private( FavoriteCollectionsModel *parent )
      : q( parent )
    {
    }

    QString labelForCollection( const Collection &c )
    {
      if ( labelMap.contains( c.id() ) ) {
        return labelMap[c.id()];
      }

      EntityTreeModel *model = qobject_cast<EntityTreeModel*>( q->sourceModel() );
      Q_ASSERT( model!=0 );

      QModelIndex index = model->indexForCollection( model->collectionForId( c.id() ) );
      return model->data(index).toString();
    }

    void clearAndUpdateSelection()
    {
      q->selectionModel()->clear();
      updateSelection();
    }

    void updateSelection()
    {
      EntityTreeModel *model = qobject_cast<EntityTreeModel*>( q->sourceModel() );
      Q_ASSERT( model!=0 );

      foreach (const Collection &c, collections) {
        QModelIndex index = model->indexForCollection( model->collectionForId( c.id() ) );
        q->selectionModel()->select( index,
                                     QItemSelectionModel::Select );
      }
    }

    void loadConfig()
    {
      KSharedConfigPtr config = KGlobal::config();
      KConfigGroup group = config->group( "FavoriteCollectionsModel" );
      QList<qint64> ids = group.readEntry( "FavoriteCollectionIds", QList<qint64>() );
      QStringList labels = group.readEntry( "FavoriteCollectionLabels", QStringList() );

      for ( int i=0; i<ids.size(); ++i ) {
        collections << Collection( ids[i] );
        if ( i<labels.size() ) {
          labelMap[ ids[i] ] = labels[i];
        }
      }

    }

    void saveConfig()
    {
      QList<qint64> ids;
      QStringList labels;

      foreach ( const Collection &c, collections ) {
        ids << c.id();
        labels << labelForCollection( c );
      }

      KSharedConfigPtr config = KGlobal::config();
      KConfigGroup group = config->group( "FavoriteCollectionsModel" );
      group.writeEntry( "FavoriteCollectionIds", ids );
      group.writeEntry( "FavoriteCollectionLabels", labels );
      config->sync();
    }

    FavoriteCollectionsModel * const q;

    Collection::List collections;
    QHash<qint64, QString> labelMap;
};

FavoriteCollectionsModel::FavoriteCollectionsModel( EntityTreeModel *source, QObject *parent )
  : SelectionProxyModel( new QItemSelectionModel( source, parent ), parent ),
    d( new Private( this ) )
{
  setSourceModel( source );
  setOmitChildren( true );
  setIncludeAllSelected( true );

  connect( source, SIGNAL( modelReset() ), this, SLOT( clearAndUpdateSelection() ) );
  connect( source, SIGNAL( layoutChanged() ), this, SLOT( clearAndUpdateSelection() ) );
  connect( source, SIGNAL( rowsInserted(QModelIndex, int, int) ), this, SLOT( updateSelection() ) );

  d->loadConfig();
  d->clearAndUpdateSelection();
}

FavoriteCollectionsModel::~FavoriteCollectionsModel()
{
  delete d;
}

void FavoriteCollectionsModel::setCollections( const Collection::List &collections )
{
  d->collections = collections;
  d->labelMap.clear();
  d->clearAndUpdateSelection();
  d->saveConfig();
}

void FavoriteCollectionsModel::addCollection( const Collection &collection )
{
  d->collections << collection;
  d->updateSelection();
  d->saveConfig();
}

void FavoriteCollectionsModel::removeCollection( const Collection &collection )
{
  d->collections.removeAll( collection );
  d->labelMap.remove( collection.id() );

  EntityTreeModel *model = qobject_cast<EntityTreeModel*>( sourceModel() );
  Q_ASSERT( model!=0 );

  QModelIndex index = model->indexForCollection( model->collectionForId( collection.id() ) );
  selectionModel()->select( index,
                            QItemSelectionModel::Deselect );

  d->updateSelection();
  d->saveConfig();
}

Collection::List FavoriteCollectionsModel::collections() const
{
  return d->collections;
}

void Akonadi::FavoriteCollectionsModel::setFavoriteLabel( const Collection &collection, const QString &label )
{
  Q_ASSERT( d->collections.contains( collection ) );
  d->labelMap[ collection.id() ] = label;
  d->saveConfig();

  EntityTreeModel *model = qobject_cast<EntityTreeModel*>( sourceModel() );
  Q_ASSERT( model!=0 );

  QModelIndex sourceIndex = model->indexForCollection( model->collectionForId( collection.id() ) );
  QModelIndex index = mapFromSource( sourceIndex );
  emit dataChanged( index, index );
}

QVariant Akonadi::FavoriteCollectionsModel::data( const QModelIndex &index, int role) const
{
  if ( index.column()==0 && role == Qt::DisplayRole ) {
    QModelIndex sourceIndex = mapToSource( index );
    Collection c = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();

    return d->labelForCollection( c );
  } else {
    return SelectionProxyModel::data( index, role );
  }
}

QVariant FavoriteCollectionsModel::headerData( int section, Qt::Orientation orientation, int role) const
{
  if ( section == 0
    && orientation == Qt::Horizontal
    && role == Qt::DisplayRole ) {
    return "Favorite Folders";
  } else {
    return SelectionProxyModel::headerData( section, orientation, role );
  }
}

#include "favoritecollectionsmodel.moc"

