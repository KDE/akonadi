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
#include <klocale.h>

#include "entitytreemodel.h"

using namespace Akonadi;

/**
 * @internal
 */
class FavoriteCollectionsModel::Private
{
  public:
    Private( const KConfigGroup &group, FavoriteCollectionsModel *parent )
      : q( parent ), configGroup( group )
    {
    }

    QString labelForCollection( const Collection &collection )
    {
      if ( labelMap.contains( collection.id() ) ) {
        return labelMap[ collection.id() ];
      }

      const QModelIndexList indexList = q->sourceModel()->match( QModelIndex(), EntityTreeModel::CollectionIdRole, collection.id() );
      Q_ASSERT( indexList.size() == 1 );

      QString accountName;

      const QString nameOfCollection = indexList.at( 0 ).data().toString();

      QModelIndex idx = indexList.at( 0 ).parent();
      while ( idx != QModelIndex() ) {
        accountName = idx.data().toString();
        idx = idx.parent();
      }

      if ( accountName.isEmpty() )
        return nameOfCollection;
      else
        return nameOfCollection + QLatin1String( " (" ) + accountName + QLatin1Char( ')' );
    }

    void clearAndUpdateSelection()
    {
      q->selectionModel()->clear();
      updateSelection();
    }

    void updateSelection()
    {
      foreach ( const Collection &collection, collections ) {
        const QModelIndexList indexList = q->sourceModel()->match( QModelIndex(), EntityTreeModel::CollectionIdRole, collection.id() );
        if ( indexList.isEmpty() )
          continue;

        Q_ASSERT( indexList.size() == 1 );
        q->selectionModel()->select( indexList.at( 0 ),
                                     QItemSelectionModel::Select );
      }
    }

    void loadConfig()
    {
      const QList<qint64> ids = configGroup.readEntry( "FavoriteCollectionIds", QList<qint64>() );
      const QStringList labels = configGroup.readEntry( "FavoriteCollectionLabels", QStringList() );

      for ( int i = 0; i < ids.size(); ++i ) {
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

      foreach ( const Collection &collection, collections ) {
        ids << collection.id();
        labels << labelForCollection( collection );
      }

      configGroup.writeEntry( "FavoriteCollectionIds", ids );
      configGroup.writeEntry( "FavoriteCollectionLabels", labels );
      configGroup.config()->sync();
    }

    FavoriteCollectionsModel * const q;

    Collection::List collections;
    QHash<qint64, QString> labelMap;
    KConfigGroup configGroup;
};

FavoriteCollectionsModel::FavoriteCollectionsModel( QAbstractItemModel *source, const KConfigGroup &group, QObject *parent )
  : Akonadi::SelectionProxyModel( new QItemSelectionModel( source, parent ), parent ),
    d( new Private( group, this ) )
{
  setSourceModel( source );
  setFilterBehavior( ExactSelection );

  connect( source, SIGNAL( modelReset() ), this, SLOT( clearAndUpdateSelection() ) );
  connect( source, SIGNAL( layoutChanged() ), this, SLOT( clearAndUpdateSelection() ) );
  connect( source, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( updateSelection() ) );

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

  const QModelIndexList indexList = sourceModel()->match( QModelIndex(), EntityTreeModel::CollectionIdRole, collection.id() );
  if ( indexList.isEmpty() )
    return;

  Q_ASSERT( indexList.size() == 1 );
  selectionModel()->select( indexList.at( 0 ),
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

  const QModelIndexList indexList = sourceModel()->match( QModelIndex(), EntityTreeModel::CollectionIdRole, collection.id() );
  if ( indexList.isEmpty() )
    return;

  Q_ASSERT( indexList.size() == 1 );

  const QModelIndex index = mapFromSource( indexList.at( 0 ) );
  emit dataChanged( index, index );
}

QVariant Akonadi::FavoriteCollectionsModel::data( const QModelIndex &index, int role ) const
{
  if ( index.column() == 0 && role == Qt::DisplayRole ) {
    const QModelIndex sourceIndex = mapToSource( index );
    const Collection collection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();

    return d->labelForCollection( collection );
  } else {
    return KSelectionProxyModel::data( index, role );
  }
}

QString Akonadi::FavoriteCollectionsModel::favoriteLabel( const Akonadi::Collection & collection )
{
  if ( !collection.isValid() )
    return QString();
  return d->labelForCollection( collection );
}

QVariant FavoriteCollectionsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( section == 0
    && orientation == Qt::Horizontal
    && role == Qt::DisplayRole ) {
    return i18n( "Favorite Folders" );
  } else {
    return KSelectionProxyModel::headerData( section, orientation, role );
  }
}

#include "favoritecollectionsmodel.moc"
