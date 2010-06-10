/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "standardactionmanager.h"

#include "agentmanager.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionmodel.h"
#include "collectionutils_p.h"
#include "collectionpropertiesdialog.h"
#include "entitytreemodel.h"
#include "favoritecollectionsmodel.h"
#include "itemdeletejob.h"
#include "itemmodel.h"
#include "pastehelper_p.h"
#include "subscriptiondialog_p.h"

#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KDebug>
#include <KInputDialog>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>

#include <QtCore/QMimeData>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QItemSelectionModel>

#include <boost/static_assert.hpp>

Q_DECLARE_METATYPE(QModelIndex)

using namespace Akonadi;

//@cond PRIVATE

static const struct {
  const char *name;
  const char *label;
  const char *icon;
  int shortcut;
  const char* slot;
  bool isActionMenu;
} actionData[] = {
  { "akonadi_collection_create", I18N_NOOP( "&New Folder..." ), "folder-new", 0, SLOT( slotCreateCollection() ), false },
  { "akonadi_collection_copy", 0, "edit-copy", 0, SLOT( slotCopyCollections() ), false },
  { "akonadi_collection_delete", I18N_NOOP( "&Delete Folder" ), "edit-delete", 0, SLOT( slotDeleteCollection() ), false },
  { "akonadi_collection_sync", I18N_NOOP( "&Synchronize Folder" ), "view-refresh", Qt::Key_F5, SLOT( slotSynchronizeCollection() ), false },
  { "akonadi_collection_properties", I18N_NOOP( "Folder &Properties" ), "configure", 0, SLOT( slotCollectionProperties() ), false },
  { "akonadi_item_copy", 0, "edit-copy", 0, SLOT( slotCopyItems() ), false },
  { "akonadi_paste", I18N_NOOP( "&Paste" ), "edit-paste", Qt::CTRL + Qt::Key_V, SLOT( slotPaste() ), false },
  { "akonadi_item_delete", 0, "edit-delete", Qt::Key_Delete, SLOT( slotDeleteItems() ), false },
  { "akonadi_manage_local_subscriptions", I18N_NOOP( "Manage Local &Subscriptions..." ), 0, 0, SLOT( slotLocalSubscription() ), false },
  { "akonadi_collection_add_to_favorites", I18N_NOOP( "Add to Favorite Folders" ), "bookmark-new", 0, SLOT( slotAddToFavorites() ), false },
  { "akonadi_collection_remove_from_favorites", I18N_NOOP( "Remove from Favorite Folders" ), "edit-delete", 0, SLOT( slotRemoveFromFavorites() ), false },
  { "akonadi_collection_rename_favorite", I18N_NOOP( "Rename Favorite..." ), "edit-rename", 0, SLOT( slotRenameFavorite() ), false },
  { "akonadi_collection_copy_to_menu", I18N_NOOP( "Copy Folder To..." ), "edit-copy", 0, SLOT( slotCopyCollectionTo( QAction* ) ), true },
  { "akonadi_item_copy_to_menu", I18N_NOOP( "Copy Item To..." ), "edit-copy", 0, SLOT( slotCopyItemTo( QAction* ) ), true },
  { "akonadi_item_move_to_menu", I18N_NOOP( "Move Item To..." ), "go-jump", 0, SLOT( slotMoveItemTo( QAction* ) ), true },
  { "akonadi_collection_move_to_menu", I18N_NOOP( "Move Folder To..." ), "go-jump", 0, SLOT( slotMoveCollectionTo( QAction* ) ), true },
  { "akonadi_item_cut", I18N_NOOP( "&Cut Item" ), "edit-cut", Qt::CTRL + Qt::Key_X, SLOT( slotCutItems() ), false },
  { "akonadi_collection_cut", I18N_NOOP( "&Cut Folder" ), "edit-cut", Qt::CTRL + Qt::Key_X, SLOT( slotCutCollections() ), false }
};
static const int numActionData = sizeof actionData / sizeof *actionData;

BOOST_STATIC_ASSERT( numActionData == StandardActionManager::LastType );

static bool canCreateCollection( const Collection &collection )
{
  if ( !( collection.rights() & Collection::CanCreateCollection ) )
    return false;

  if ( !collection.contentMimeTypes().contains( Collection::mimeType() ) )
    return false;

  return true;
}

static inline bool isRootCollection( const Collection &collection )
{
  return (collection == Collection::root());
}

/**
 * @internal
 */
class StandardActionManager::Private
{
  public:
    Private( StandardActionManager *parent ) :
      q( parent ),
      collectionSelectionModel( 0 ),
      itemSelectionModel( 0 ),
      favoritesModel( 0 ),
      favoriteSelectionModel( 0 )
    {
      actions.fill( 0, StandardActionManager::LastType );

      pluralLabels.insert( StandardActionManager::CopyCollections, ki18np( "&Copy Folder", "&Copy %1 Folders" ) );
      pluralLabels.insert( StandardActionManager::CopyItems, ki18np( "&Copy Item", "&Copy %1 Items" ) );
      pluralLabels.insert( StandardActionManager::CutItems, ki18np( "&Cut Item", "&Cut %1 Items" ) );
      pluralLabels.insert( StandardActionManager::CutCollections, ki18np( "&Cut Folder", "&Cut %1 Folders" ) );
      pluralLabels.insert( StandardActionManager::DeleteItems, ki18np( "&Delete Item", "&Delete %1 Items" ) );
    }

    void enableAction( StandardActionManager::Type type, bool enable )
    {
      Q_ASSERT( type >= 0 && type < StandardActionManager::LastType );
      if ( actions[type] )
        actions[type]->setEnabled( enable );

      // Update the action menu
      KActionMenu *actionMenu = qobject_cast<KActionMenu*>( actions[type] );
      if ( actionMenu ) {
        actionMenu->menu()->clear();
        if ( enable ) {
          fillFoldersMenu( type,
                           actionMenu->menu(),
                           collectionSelectionModel->model(),
                           QModelIndex() );
        }
      }
    }

    void updatePluralLabel( StandardActionManager::Type type, int count )
    {
      Q_ASSERT( type >= 0 && type < StandardActionManager::LastType );
      if ( actions[type] && pluralLabels.contains( type ) && !pluralLabels.value( type ).isEmpty() ) {
        actions[type]->setText( pluralLabels.value( type ).subs( qMax( count, 1 ) ).toString() );
      }
    }

    void encodeToClipboard( QItemSelectionModel* selectionModel, bool cut = false )
    {
      Q_ASSERT( selectionModel );
      if ( selectionModel->selectedRows().count() <= 0 )
        return;

      QMimeData *mimeData = selectionModel->model()->mimeData( selectionModel->selectedRows() );
      markCutAction( mimeData, cut );
      QApplication::clipboard()->setMimeData( mimeData );

      QAbstractItemModel *model = const_cast<QAbstractItemModel *>( selectionModel->model() );

      foreach ( const QModelIndex &index, selectionModel->selectedRows() )
        model->setData( index, true, EntityTreeModel::PendingCutRole );
    }

    void updateActions()
    {
      bool singleCollectionSelected = false;
      bool multipleCollectionsSelected = false;
      bool canDeleteCollections = true;
      int collectionCount = 0;

      QModelIndex selectedIndex;
      if ( !collectionSelectionModel ) {
        canDeleteCollections = false;
      } else {
        collectionCount = collectionSelectionModel->selectedRows().count();
        singleCollectionSelected = collectionCount == 1;
        multipleCollectionsSelected = collectionCount > 1;
        canDeleteCollections = collectionCount > 0;

        if ( singleCollectionSelected )
          selectedIndex = collectionSelectionModel->selectedRows().first();

        if ( itemSelectionModel ) {
          const QModelIndexList rows = itemSelectionModel->selectedRows();
          foreach ( const QModelIndex &itemIndex, rows ) {
            const Collection collection = itemIndex.data( EntityTreeModel::CollectionRole ).value<Collection>();
            if ( !collection.isValid() )
              continue;

            if ( collection == collection.root() )
              // The root collection is selected. There are no valid actions to enable.
              return;

            canDeleteCollections = canDeleteCollections && ( collection.rights() & Collection::CanDeleteCollection );
          }
        }
      }

      const Collection collection = selectedIndex.data( CollectionModel::CollectionRole ).value<Collection>();

      enableAction( CopyCollections, (singleCollectionSelected || multipleCollectionsSelected) && !isRootCollection( collection ) );
      enableAction( CollectionProperties, singleCollectionSelected && !isRootCollection( collection ) );

      enableAction( CreateCollection, singleCollectionSelected && canCreateCollection( collection ) );
      enableAction( DeleteCollections, singleCollectionSelected && (collection.rights() & Collection::CanDeleteCollection) && !CollectionUtils::isResource( collection ) );
      enableAction( CutCollections, canDeleteCollections && !isRootCollection( collection ) && !CollectionUtils::isResource( collection ) );
      enableAction( SynchronizeCollections, singleCollectionSelected && (CollectionUtils::isResource( collection ) || CollectionUtils::isFolder( collection ) ) );
      enableAction( Paste, singleCollectionSelected && PasteHelper::canPaste( QApplication::clipboard()->mimeData(), collection ) );
      enableAction( AddToFavoriteCollections, singleCollectionSelected && ( favoritesModel != 0 ) && ( !favoritesModel->collections().contains( collection ) ) );
      enableAction( RemoveFromFavoriteCollections, singleCollectionSelected && ( favoritesModel != 0 ) && ( favoritesModel->collections().contains( collection ) ) );
      enableAction( RenameFavoriteCollection, singleCollectionSelected && ( favoritesModel != 0 ) && ( favoritesModel->collections().contains( collection ) ) );
      enableAction( CopyCollectionToMenu, (singleCollectionSelected || multipleCollectionsSelected) && !isRootCollection( collection ) );
      enableAction( MoveCollectionToMenu, canDeleteCollections && !isRootCollection( collection ) && !CollectionUtils::isResource( collection ) );

      bool multipleItemsSelected = false;
      bool canDeleteItems = true;
      int itemCount = 0;
      if ( !itemSelectionModel ) {
        canDeleteItems = false;
      } else {
        const QModelIndexList rows = itemSelectionModel->selectedRows();

        itemCount = rows.count();
        multipleItemsSelected = itemCount > 0;
        canDeleteItems = itemCount > 0;

        foreach ( const QModelIndex &itemIndex, rows ) {
          const Collection parentCollection = itemIndex.data( EntityTreeModel::ParentCollectionRole ).value<Collection>();
          if ( !parentCollection.isValid() )
            continue;

          canDeleteItems = canDeleteItems && (parentCollection.rights() & Collection::CanDeleteItem);
        }
      }

      enableAction( CopyItems, multipleItemsSelected );
      enableAction( CutItems, canDeleteItems );

      enableAction( DeleteItems, multipleItemsSelected && canDeleteItems );

      enableAction( CopyItemToMenu, multipleItemsSelected );
      enableAction( MoveItemToMenu, multipleItemsSelected && canDeleteItems );

      updatePluralLabel( CopyCollections, collectionCount );
      updatePluralLabel( CopyItems, itemCount );
      updatePluralLabel( DeleteItems, itemCount );
      updatePluralLabel( CutItems, itemCount );
      updatePluralLabel( CutCollections, itemCount );

      emit q->actionStateUpdated();
    }

    void clipboardChanged( QClipboard::Mode mode )
    {
      if ( mode == QClipboard::Clipboard )
        updateActions();
    }

    QItemSelection mapToEntityTreeModel( const QAbstractItemModel *model, const QItemSelection &selection ) const
    {
      const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>( model );
      if ( proxy ) {
        return mapToEntityTreeModel( proxy->sourceModel(), proxy->mapSelectionToSource( selection ) );
      } else {
        return selection;
      }
    }

    QItemSelection mapFromEntityTreeModel( const QAbstractItemModel *model, const QItemSelection &selection ) const
    {
      const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>( model );
      if ( proxy ) {
        const QItemSelection select = mapFromEntityTreeModel( proxy->sourceModel(), selection );
        return proxy->mapSelectionFromSource( select );
      } else {
        return selection;
      }
    }

    void collectionSelectionChanged()
    {
      q->blockSignals( true );

      QItemSelection selection = collectionSelectionModel->selection();
      selection = mapToEntityTreeModel( collectionSelectionModel->model(), selection );
      selection = mapFromEntityTreeModel( favoritesModel, selection );

      if ( favoriteSelectionModel )
        favoriteSelectionModel->select( selection, QItemSelectionModel::ClearAndSelect );

      q->blockSignals( false );

      updateActions();
    }

    void favoriteSelectionChanged()
    {
      q->blockSignals( true );

      QItemSelection selection = favoriteSelectionModel->selection();
      if ( selection.indexes().isEmpty() )
        return;

      selection = mapToEntityTreeModel( favoritesModel, selection );
      selection = mapFromEntityTreeModel( collectionSelectionModel->model(), selection );

      collectionSelectionModel->select( selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );

      q->blockSignals( false );

      updateActions();
    }

    void slotCreateCollection()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection parentCollection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( parentCollection.isValid() );

      if ( !canCreateCollection( parentCollection ) )
        return;

      const QString name = KInputDialog::getText( i18nc( "@title:window", "New Folder" ),
                                                  i18nc( "@label:textbox name of a thing", "Name" ),
                                                  QString(), 0, parentWidget );
      if ( name.isEmpty() )
        return;

      Collection collection;
      collection.setName( name );
      collection.setParentCollection( parentCollection );
      CollectionCreateJob *job = new CollectionCreateJob( collection );
      q->connect( job, SIGNAL( result( KJob* ) ), q, SLOT( collectionCreationResult( KJob* ) ) );
    }

    void slotCopyCollections()
    {
      encodeToClipboard( collectionSelectionModel );
    }

    void slotCutCollections()
    {
      encodeToClipboard( collectionSelectionModel, true );
    }

    void slotDeleteCollection()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      QString text = i18n( "Do you really want to delete folder '%1' and all its sub-folders?", index.data().toString() );
      if ( CollectionUtils::isVirtual( collection ) )
        text = i18n( "Do you really want to delete the search view '%1'?", index.data().toString() );

      if ( KMessageBox::questionYesNo( parentWidget, text,
           i18n( "Delete folder?" ), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
           QString(), KMessageBox::Dangerous ) != KMessageBox::Yes )
        return;

      const Collection::Id collectionId = index.data( CollectionModel::CollectionIdRole ).toLongLong();
      if ( collectionId <= 0 )
        return;

      CollectionDeleteJob *job = new CollectionDeleteJob( Collection( collectionId ), q );
      q->connect( job, SIGNAL( result( KJob* ) ), q, SLOT( collectionDeletionResult( KJob* ) ) );
    }

    void slotSynchronizeCollection()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      AgentManager::self()->synchronizeCollection( collection );
    }

    void slotCollectionProperties()
    {
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;
      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      CollectionPropertiesDialog* dlg = new CollectionPropertiesDialog( collection, parentWidget );
      dlg->setCaption( i18n( "Properties of Folder %1", collection.name() ) );
      dlg->show();
    }

    void slotCopyItems()
    {
      encodeToClipboard( itemSelectionModel );
    }

    void slotCutItems()
    {
      encodeToClipboard( itemSelectionModel, true );
    }

    void slotPaste()
    {
      Q_ASSERT( collectionSelectionModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );

      // TODO: Copy or move? We can't seem to cut yet
      QAbstractItemModel *model = const_cast<QAbstractItemModel *>( collectionSelectionModel->model() );
      const QMimeData *mimeData = QApplication::clipboard()->mimeData();
      model->dropMimeData( mimeData, isCutAction( mimeData ) ? Qt::MoveAction : Qt::CopyAction, -1, -1, index );
      model->setData( QModelIndex(), false, EntityTreeModel::PendingCutRole );
      QApplication::clipboard()->clear();
    }

    void slotDeleteItems()
    {
      if ( KMessageBox::questionYesNo( parentWidget,
           i18n( "Do you really want to delete all selected items?" ),
           i18n( "Delete?" ), KStandardGuiItem::del(), KStandardGuiItem::cancel(),
           QString(), KMessageBox::Dangerous ) != KMessageBox::Yes )
        return;

      Q_ASSERT( itemSelectionModel );

      // TODO: fix this once ItemModifyJob can handle item lists
      Item::List items;
      foreach ( const QModelIndex &index, itemSelectionModel->selectedRows() ) {
        bool ok;
        const qlonglong id = index.data( ItemModel::IdRole ).toLongLong( &ok );
        Q_ASSERT( ok );
        items << Item( id );
      }

      new ItemDeleteJob( items, q );
    }

    void slotLocalSubscription()
    {
      SubscriptionDialog* dlg = new SubscriptionDialog( parentWidget );
      dlg->show();
    }

    void slotAddToFavorites()
    {
      Q_ASSERT( collectionSelectionModel );
      Q_ASSERT( favoritesModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      favoritesModel->addCollection( collection );
      enableAction( AddToFavoriteCollections, false );
    }

    void slotRemoveFromFavorites()
    {
      Q_ASSERT( collectionSelectionModel );
      Q_ASSERT( favoritesModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      favoritesModel->removeCollection( collection );
      if ( favoritesModel->collections().count() <= 1 )
        enableAction( AddToFavoriteCollections, true );
    }

    void slotRenameFavorite()
    {
      Q_ASSERT( collectionSelectionModel );
      Q_ASSERT( favoritesModel );
      if ( collectionSelectionModel->selection().indexes().isEmpty() )
        return;

      const QModelIndex index = collectionSelectionModel->selection().indexes().at( 0 );
      Q_ASSERT( index.isValid() );
      const Collection collection = index.data( CollectionModel::CollectionRole ).value<Collection>();
      Q_ASSERT( collection.isValid() );

      bool ok;
      const QString label = KInputDialog::getText( i18n( "Rename Favorite" ),
                                                   i18nc( "@label:textbox New name of the folder.", "Name:" ),
                                                   favoritesModel->favoriteLabel( collection ), &ok, parentWidget );
      if ( !ok )
        return;

      favoritesModel->setFavoriteLabel( collection, label );
    }

    void slotCopyCollectionTo( QAction *action )
    {
      pasteTo( collectionSelectionModel, action, Qt::CopyAction );
    }

    void slotCopyItemTo( QAction *action )
    {
      pasteTo( itemSelectionModel, action, Qt::CopyAction );
    }

    void slotMoveCollectionTo( QAction *action )
    {
      pasteTo( collectionSelectionModel, action, Qt::MoveAction );
    }

    void slotMoveItemTo( QAction *action )
    {
      pasteTo( itemSelectionModel, action, Qt::MoveAction );
    }

    void pasteTo( QItemSelectionModel *selectionModel, QAction *action, Qt::DropAction dropAction )
    {
      Q_ASSERT( selectionModel );
      Q_ASSERT( action );

      if ( selectionModel->selectedRows().count() <= 0 )
        return;

      const QMimeData *mimeData = selectionModel->model()->mimeData( selectionModel->selectedRows() );

      const QModelIndex index = action->data().value<QModelIndex>();

      Q_ASSERT( index.isValid() );

      QAbstractItemModel *model = const_cast<QAbstractItemModel *>( index.model() );
      model->dropMimeData( mimeData, dropAction, -1, -1, index );
    }

    void collectionCreationResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n( "Could not create folder: %1", job->errorString()),
                            i18n( "Folder creation failed" ) );
      }
    }

    void collectionDeletionResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n( "Could not delete folder: %1", job->errorString()),
                            i18n( "Folder deletion failed" ) );
      }
    }

    void pasteResult( KJob *job )
    {
      if ( job->error() ) {
        KMessageBox::error( parentWidget, i18n( "Could not paste data: %1", job->errorString()),
                            i18n( "Paste failed" ) );
      }
    }

    void fillFoldersMenu( StandardActionManager::Type type, QMenu *menu,
                          const QAbstractItemModel *model, QModelIndex parentIndex )
    {
      const int rowCount = model->rowCount( parentIndex );

      QModelIndexList list;
      QSet<QString> mimeTypes;

      const bool isItemAction = ( type == CopyItemToMenu || type == MoveItemToMenu );
      const bool isCollectionAction = ( type == CopyCollectionToMenu || type == MoveCollectionToMenu );

      if ( isItemAction ) {
        list = itemSelectionModel->selectedRows();
        foreach ( const QModelIndex &index, list )
          mimeTypes << index.data( EntityTreeModel::MimeTypeRole ).toString();
      }

      if ( isCollectionAction ) {
        list = collectionSelectionModel->selectedRows();
        foreach ( const QModelIndex &index, list ) {
          const Collection collection = index.data( EntityTreeModel::CollectionRole ).value<Collection>();

          // The mimetypes that the selected collection can possibly contain
          mimeTypes = AgentManager::self()->type( collection.resource() ).mimeTypes().toSet();
        }
      }

      for ( int row = 0; row < rowCount; ++row ) {
        const QModelIndex index = model->index( row, 0, parentIndex );
        const Collection collection = model->data( index, CollectionModel::CollectionRole ).value<Collection>();

        if ( CollectionUtils::isVirtual( collection ) ) {
          continue;
        }

        QString label = model->data( index ).toString();
        label.replace( QLatin1String( "&" ), QLatin1String( "&&" ) );

        const QIcon icon = model->data( index, Qt::DecorationRole ).value<QIcon>();

        const bool canContainRequiredMimeTypes = !collection.contentMimeTypes().toSet().intersect( mimeTypes ).isEmpty();
        const bool canCreateNewItems = (collection.rights() & Collection::CanCreateItem);


        const bool canCreateNewCollections = (collection.rights() & Collection::CanCreateCollection);
        const bool canContainCollections = collection.contentMimeTypes().contains( Collection::mimeType() );
        const bool resourceAllowsRequiredMimeTypes = !(QSet<QString>( mimeTypes ).subtract( AgentManager::self()->type( collection.resource() ).mimeTypes().toSet() ).isEmpty());

        const bool isReadOnlyForItems = (isItemAction && (!canCreateNewItems || !canContainRequiredMimeTypes));
        const bool isReadOnlyForCollections = (isCollectionAction && (!canCreateNewCollections || !canContainCollections || !resourceAllowsRequiredMimeTypes));

        const bool readOnly = CollectionUtils::isStructural( collection ) || isReadOnlyForItems || isReadOnlyForCollections;

        if ( model->rowCount( index ) > 0 ) {
          // new level
          QMenu* popup = new QMenu( menu );
          const bool moveAction = ( type == MoveCollectionToMenu || type == MoveItemToMenu);
          popup->setObjectName( QString::fromUtf8( "subMenu" ) );
          popup->setTitle( label );
          popup->setIcon( icon );

          fillFoldersMenu( type, popup, model, index );

          if ( !readOnly ) {
            popup->addSeparator();

            QAction *action = popup->addAction( moveAction ? i18n( "Move to This Folder" ) : i18n( "Copy to This Folder" ) );
            action->setData( QVariant::fromValue<QModelIndex>( index ) );
          }

          menu->addMenu( popup );

        } else {
          // insert an item
          QAction* action = menu->addAction( icon, label );
          action->setData( QVariant::fromValue<QModelIndex>( index ) );
          action->setEnabled( !readOnly );
        }
      }
    }

    void checkModelsConsistency()
    {
      if ( favoritesModel == 0 || favoriteSelectionModel == 0 ) {
        // No need to check when the favorite collections feature is not used
        return;
      }

      // Check that the collection selection model maps to the same
      // EntityTreeModel than favoritesModel
      if ( collectionSelectionModel != 0 ) {
        const QAbstractItemModel *model = collectionSelectionModel->model();
        while ( const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>( model ) ) {
          model = proxy->sourceModel();
        }
        Q_ASSERT( model == favoritesModel->sourceModel() );
      }

      // Check that the favorite selection model maps to favoritesModel
      const QAbstractItemModel *model = favoriteSelectionModel->model();
      while ( const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>( model ) ) {
        model = proxy->sourceModel();
      }
      Q_ASSERT( model == favoritesModel->sourceModel() );
    }

    void markCutAction( QMimeData *mimeData, bool cut ) const
    {
      if ( !cut )
        return;

      const QByteArray cutSelectionData = "1"; //krazy:exclude=doublequote_chars
      mimeData->setData( QLatin1String( "application/x-kde.akonadi-cutselection" ), cutSelectionData);
    }

    bool isCutAction( const QMimeData *mimeData ) const
    {
      const QByteArray data = mimeData->data( QLatin1String( "application/x-kde.akonadi-cutselection" ) );
      if ( data.isEmpty() )
        return false;
      else
        return (data.at( 0 ) == '1'); // true if 1
    }

    StandardActionManager *q;
    KActionCollection *actionCollection;
    QWidget *parentWidget;
    QItemSelectionModel *collectionSelectionModel;
    QItemSelectionModel *itemSelectionModel;
    FavoriteCollectionsModel *favoritesModel;
    QItemSelectionModel *favoriteSelectionModel;
    QVector<KAction*> actions;
    QHash<StandardActionManager::Type, KLocalizedString> pluralLabels;
};

//@endcond

StandardActionManager::StandardActionManager( KActionCollection * actionCollection,
                                              QWidget * parent) :
    QObject( parent ),
    d( new Private( this ) )
{
  d->parentWidget = parent;
  d->actionCollection = actionCollection;
  connect( QApplication::clipboard(), SIGNAL( changed( QClipboard::Mode ) ), SLOT( clipboardChanged( QClipboard::Mode ) ) );
}

StandardActionManager::~ StandardActionManager()
{
  delete d;
}

void StandardActionManager::setCollectionSelectionModel( QItemSelectionModel * selectionModel )
{
  d->collectionSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
           SLOT( collectionSelectionChanged() ) );

  d->checkModelsConsistency();
}

void StandardActionManager::setItemSelectionModel( QItemSelectionModel * selectionModel )
{
  d->itemSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
           SLOT( updateActions() ) );
}

void StandardActionManager::setFavoriteCollectionsModel( FavoriteCollectionsModel *favoritesModel )
{
  d->favoritesModel = favoritesModel;
  d->checkModelsConsistency();
}

void StandardActionManager::setFavoriteSelectionModel( QItemSelectionModel *selectionModel )
{
  d->favoriteSelectionModel = selectionModel;
  connect( selectionModel, SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
           SLOT( favoriteSelectionChanged() ) );
  d->checkModelsConsistency();
}

KAction* StandardActionManager::createAction( Type type )
{
  Q_ASSERT( type >= 0 && type < LastType );
  Q_ASSERT( actionData[type].name );
  if ( d->actions[type] )
    return d->actions[type];
  KAction *action;
  if ( !actionData[type].isActionMenu ) {
    action = new KAction( d->parentWidget );
  } else {
    action = new KActionMenu( d->parentWidget );
  }

  if ( d->pluralLabels.contains( type ) && !d->pluralLabels.value( type ).isEmpty() )
    action->setText( d->pluralLabels.value( type ).subs( 1 ).toString() );
  else if ( actionData[type].label )
    action->setText( i18n( actionData[type].label ) );

  if ( actionData[type].icon )
    action->setIcon( KIcon( QString::fromLatin1( actionData[type].icon ) ) );

  action->setShortcut( actionData[type].shortcut );

  if ( actionData[type].slot && !actionData[type].isActionMenu ) {
    connect( action, SIGNAL( triggered() ), actionData[type].slot );
  } else if ( actionData[type].slot ) {
    KActionMenu *actionMenu = qobject_cast<KActionMenu*>( action );
    connect( actionMenu->menu(), SIGNAL( triggered( QAction* ) ), actionData[type].slot );
  }

  d->actionCollection->addAction( QString::fromLatin1(actionData[type].name), action );
  d->actions[type] = action;
  d->updateActions();
  return action;
}

void StandardActionManager::createAllActions()
{
  for ( int i = 0; i < LastType; ++i )
    createAction( (Type)i );
}

KAction * StandardActionManager::action( Type type ) const
{
  Q_ASSERT( type >= 0 && type < LastType );
  return d->actions[type];
}

void StandardActionManager::setActionText( Type type, const KLocalizedString & text )
{
  Q_ASSERT( type >= 0 && type < LastType );
  d->pluralLabels.insert( type, text );
  d->updateActions();
}

#include "standardactionmanager.moc"
