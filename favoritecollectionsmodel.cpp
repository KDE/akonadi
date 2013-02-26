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

#include <QItemSelectionModel>
#include <QtCore/QMimeData>

#include <kconfiggroup.h>
#include <klocale.h>
#include <klocalizedstring.h>
#include <KJob>
#include <KUrl>

#include "entitytreemodel.h"
#include "mimetypechecker.h"
#include "pastehelper_p.h"

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

    QString labelForCollection( Collection::Id collectionId ) const
    {
      if ( labelMap.contains( collectionId ) ) {
        return labelMap[ collectionId ];
      }

      const QModelIndex collectionIdx = EntityTreeModel::modelIndexForCollection( q->sourceModel(), Collection( collectionId ) );

      QString accountName;

      const QString nameOfCollection = collectionIdx.data().toString();

      QModelIndex idx = collectionIdx.parent();
      while ( idx != QModelIndex() ) {
        accountName = idx.data().toString();
        idx = idx.parent();
      }

      if ( accountName.isEmpty() ) {
        return nameOfCollection;
      } else {
        return nameOfCollection + QLatin1String( " (" ) + accountName + QLatin1Char( ')' );
      }
    }

    void clearAndUpdateSelection()
    {
      q->selectionModel()->clear();
      updateSelection();
    }

    void updateSelectionId( const Collection::Id &collectionId )
    {
      const QModelIndex index = EntityTreeModel::modelIndexForCollection( q->sourceModel(), Collection( collectionId ) );

      if ( index.isValid() ) {
        q->selectionModel()->select( index, QItemSelectionModel::Select );
      }
    }

    void updateSelection()
    {
      foreach ( const Collection::Id &collectionId, collectionIds ) {
        updateSelectionId( collectionId );
      }
    }

    void loadConfig()
    {
      collectionIds = configGroup.readEntry( "FavoriteCollectionIds", QList<qint64>() );
      const QStringList labels = configGroup.readEntry( "FavoriteCollectionLabels", QStringList() );
      const int numberOfCollection( collectionIds.size() );
      const int numberOfLabels( labels.size() );
      for ( int i = 0; i < numberOfCollection; ++i ) {
        if ( i<numberOfLabels ) {
          labelMap[ collectionIds[i] ] = labels[i];
        }
      }
    }

    void saveConfig()
    {
      QStringList labels;

      foreach ( const Collection::Id &collectionId, collectionIds ) {
        labels << labelForCollection( collectionId );
      }

      configGroup.writeEntry( "FavoriteCollectionIds", collectionIds );
      configGroup.writeEntry( "FavoriteCollectionLabels", labels );
      configGroup.config()->sync();
    }

    FavoriteCollectionsModel * const q;

    QList<Collection::Id> collectionIds;
    QHash<qint64, QString> labelMap;
    KConfigGroup configGroup;
};

FavoriteCollectionsModel::FavoriteCollectionsModel( QAbstractItemModel *source, const KConfigGroup &group, QObject *parent )
  : Akonadi::SelectionProxyModel( new QItemSelectionModel( source, parent ), parent ),
    d( new Private( group, this ) )
{
  setSourceModel( source );
  setFilterBehavior( ExactSelection );

  d->loadConfig();
  connect( source, SIGNAL(modelReset()), this, SLOT(clearAndUpdateSelection()) );
  connect( source, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateSelection()) );
  d->updateSelection();
}

FavoriteCollectionsModel::~FavoriteCollectionsModel()
{
  delete d;
}

void FavoriteCollectionsModel::setCollections( const Collection::List &collections )
{
  d->collectionIds.clear();
  foreach ( const Collection &col, collections ) {
    d->collectionIds << col.id();
  }
  d->labelMap.clear();
  d->clearAndUpdateSelection();
  d->saveConfig();
}

void FavoriteCollectionsModel::addCollection( const Collection &collection )
{
  d->collectionIds << collection.id();
  d->updateSelectionId( collection.id() );
  d->saveConfig();
}

void FavoriteCollectionsModel::removeCollection( const Collection &collection )
{
  d->collectionIds.removeAll( collection.id() );
  d->labelMap.remove( collection.id() );

  const QModelIndex idx = EntityTreeModel::modelIndexForCollection( sourceModel(), collection );

  if ( !idx.isValid() ) {
    return;
  }

  selectionModel()->select( idx,
                            QItemSelectionModel::Deselect );

  d->updateSelection();
  d->saveConfig();
}

Akonadi::Collection::List FavoriteCollectionsModel::collections() const
{
  Collection::List cols;
  foreach ( const Collection::Id &colId, d->collectionIds ) {
    const QModelIndex idx = EntityTreeModel::modelIndexForCollection( sourceModel(), Collection( colId ) );
    const Collection collection = sourceModel()->data( idx, EntityTreeModel::CollectionRole ).value<Collection>();
    cols << collection;
  }
  return cols;
}

QList<Collection::Id> FavoriteCollectionsModel::collectionIds() const
{
  return d->collectionIds;
}

void Akonadi::FavoriteCollectionsModel::setFavoriteLabel( const Collection &collection, const QString &label )
{
  Q_ASSERT( d->collectionIds.contains( collection.id() ) );
  d->labelMap[ collection.id() ] = label;
  d->saveConfig();

  const QModelIndex idx = EntityTreeModel::modelIndexForCollection( sourceModel(), collection );

  if ( !idx.isValid() ) {
    return;
  }

  const QModelIndex index = mapFromSource( idx );
  emit dataChanged( index, index );
}

QVariant Akonadi::FavoriteCollectionsModel::data( const QModelIndex &index, int role ) const
{
  if ( index.column() == 0 &&
       ( role == Qt::DisplayRole ||
         role == Qt::EditRole ) ) {
    const QModelIndex sourceIndex = mapToSource( index );
    const Collection::Id collectionId = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionIdRole ).toLongLong();

    return d->labelForCollection( collectionId );
  } else {
    return KSelectionProxyModel::data( index, role );
  }
}

bool FavoriteCollectionsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if ( index.isValid() && index.column() == 0 &&
       role == Qt::EditRole ) {
    const QString newLabel = value.toString();
    if ( newLabel.isEmpty() ) {
      return false;
    }
    const QModelIndex sourceIndex = mapToSource( index );
    const Collection collection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();
    setFavoriteLabel( collection, newLabel );
    return true;
  }
  return Akonadi::SelectionProxyModel::setData( index, value, role );
}

QString Akonadi::FavoriteCollectionsModel::favoriteLabel( const Akonadi::Collection & collection )
{
  if ( !collection.isValid() ) {
    return QString();
  }
  return d->labelForCollection( collection.id() );
}

QVariant FavoriteCollectionsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( section == 0 &&
       orientation == Qt::Horizontal &&
       role == Qt::DisplayRole ) {
    return i18n( "Favorite Folders" );
  } else {
    return KSelectionProxyModel::headerData( section, orientation, role );
  }
}

bool FavoriteCollectionsModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
  Q_UNUSED( action );
  Q_UNUSED( row );
  Q_UNUSED( column );
  if ( data->hasFormat( QLatin1String( "text/uri-list" ) ) ) {
    const KUrl::List urls = KUrl::List::fromMimeData( data );

    const QModelIndex sourceIndex = mapToSource( parent );
    const Collection destCollection = sourceModel()->data( sourceIndex, EntityTreeModel::CollectionRole ).value<Collection>();

    MimeTypeChecker mimeChecker;
    mimeChecker.setWantedMimeTypes( destCollection.contentMimeTypes() );

    foreach ( const KUrl &url, urls ) {
      const Collection col = Collection::fromUrl( url );
      if ( col.isValid() ) {
        addCollection( col );
      } else {
        const Item item = Item::fromUrl( url );
        if ( item.isValid() ) {
          if ( item.parentCollection().id() == destCollection.id() &&
               action != Qt::CopyAction ) {
            kDebug() << "Error: source and destination of move are the same.";
            return false;
          }
#if 0
          if ( !mimeChecker.isWantedItem( item ) ) {
            kDebug() << "unwanted item" << mimeChecker.wantedMimeTypes() << item.mimeType();
            return false;
          }
#endif
          KJob *job = PasteHelper::pasteUriList( data, destCollection, action );
          if ( !job ) {
            return false;
          }
          connect( job, SIGNAL(result(KJob*)), SLOT(pasteJobDone(KJob*)) );
          // Accept the event so that it doesn't propagate.
          return true;

        }
      }

    }
    return true;
  }
  return false;
}

QStringList FavoriteCollectionsModel::mimeTypes() const
{
  QStringList mts = Akonadi::SelectionProxyModel::mimeTypes();
  if ( !mts.contains( QLatin1String( "text/uri-list" ) ) ) {
    mts.append( QLatin1String( "text/uri-list" ) );
  }
  return mts;
}

Qt::ItemFlags FavoriteCollectionsModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags fs = Akonadi::SelectionProxyModel::flags( index );
  if ( !index.isValid() ) {
    fs |= Qt::ItemIsDropEnabled;
  }
  return fs;
}

void FavoriteCollectionsModel::pasteJobDone( KJob *job )
{
  if ( job->error() ) {
    kDebug() << job->errorString();
  }
}

#include "moc_favoritecollectionsmodel.cpp"
