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

#ifndef AKONADI_STANDARDACTIONMANAGER_H
#define AKONADI_STANDARDACTIONMANAGER_H

#include "akonadi_export.h"

#include <QtCore/QObject>

class KAction;
class KActionCollection;
class KLocalizedString;
class QItemSelectionModel;
class QWidget;

namespace Akonadi {

class FavoriteCollectionsModel;

/**
 * @short Manages generic actions for collection and item views.
 *
 * Manages generic Akonadi actions common for all types. This covers
 * creating of the actions with appropriate labels, icons, shortcuts
 * etc., updating the action state depending on the current selection
 * as well as default implementations for the actual operations.
 *
 * If the default implementation is not appropriate for your application
 * you can still use the state tracking by disconnecting the triggered()
 * signal and re-connecting it to your implementation. The actual KAction
 * objects can be retrieved by calling createAction() or action() for that.
 *
 * If the default look and feel (labels, icons, shortcuts) of the actions
 * is not appropriate for your application, you can access them as noted
 * above and customize them to your needs. Additionally, you can set a
 * KLocalizedString which should be used as a action label with correct
 * plural handling for actions operating on multiple objects with
 * setActionText().
 *
 * Finally, if you have special needs for the action states, connect to
 * the actionStateUpdated() signal and adjust the state accordingly.
 *
 * The following actions are provided (KAction name in parenthesis):
 * - Creation of a new collection (@c akonadi_collection_create)
 * - Copying of selected collections (@c akonadi_collection_copy)
 * - Deletion of selected collections (@c akonadi_collection_delete)
 * - Synchronization of selected collections (@c akonadi_collection_sync)
 * - Showing the collection properties dialog for the current collection (@c akonadi_collection_properties)
 * - Copying of selected items (@c akonadi_itemcopy)
 * - Pasting collections, items or raw data (@c akonadi_paste)
 * - Deleting of selected items (@c akonadi_item_delete)
 * - Managing local subscriptions (@c akonadi_manage_local_subscriptions)
 *
 * The following example shows how to use standard actions in your application:
 *
 * @code
 *
 * Akonadi::StandardActionManager *actMgr = new Akonadi::StandardActionManager( actionCollection(), this );
 * actMgr->setCollectionSelectionModel( collectionView->collectionSelectionModel() );
 * actMgr->createAllActions();
 *
 * @endcode
 *
 * Additionally you have to add the actions to the KXMLGUI file of your application,
 * using the names listed above.
 *
 * If you only need a subset of the actions provided, you can call createAction()
 * instead of createAllActions() for the action types you want.
 *
 * @todo collection deleting and sync do not support multi-selection yet
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT StandardActionManager : public QObject
{
  Q_OBJECT
  public:
    /**
     * Describes the supported actions.
     */
    enum Type {
      CreateCollection,          ///< Creates an collection
      CopyCollections,           ///< Copies the selected collections
      DeleteCollections,         ///< Deletes the selected collections
      SynchronizeCollections,    ///< Synchronizes collections
      CollectionProperties,      ///< Provides collection properties
      CopyItems,                 ///< Copies the selected items
      Paste,                     ///< Paste collections or items
      DeleteItems,               ///< Deletes the selected items
      ManageLocalSubscriptions,  ///< Manages local subscriptions
      AddToFavoriteCollections,  ///< Add the collection to the favorite collections model @since 4.4
      RemoveFromFavoriteCollections,  ///< Remove the collection from the favorite collections model @since 4.4
      RenameFavoriteCollection,  ///< Rename the collection of the favorite collections model @since 4.4
      CopyCollectionToMenu,      ///< Menu allowing to quickly copy a collection into another collection @since 4.4
      CopyItemToMenu,            ///< Menu allowing to quickly copy an item into a collection @since 4.4
      MoveItemToMenu,            ///< Menu allowing to move item into a collection @since 4.4
      MoveCollectionToMenu,      ///< Menu allowing to move a collection into another collection @since 4.4
      CutItems,                  ///< Cuts the selected items @since 4.4
      CutCollections,            ///< Cuts the selected collections @since 4.4
      LastType                   ///< Marks last action
    };

    /**
     * Creates a new standard action manager.
     *
     * @param actionCollection The action collection to operate on.
     * @param parent The parent widget.
     */
    explicit StandardActionManager( KActionCollection *actionCollection, QWidget *parent = 0 );

    /**
     * Destroys the standard action manager.
     */
    ~StandardActionManager();

    /**
     * Sets the collection selection model based on which the collection
     * related actions should operate. If none is set, all collection actions
     * will be disabled.
     */
    void setCollectionSelectionModel( QItemSelectionModel *selectionModel );

    /**
     * Sets the item selection model based on which the item related actions
     * should operate. If none is set, all item actions will be disabled.
     */
    void setItemSelectionModel( QItemSelectionModel* selectionModel );

    /**
     * Sets the favorite collections model based on which the collection
     * relatedactions should operate. If none is set, the "Add to Favorite Folders" action
     * will be disabled.
     *
     * @since 4.4
     */
    void setFavoriteCollectionsModel( FavoriteCollectionsModel *favoritesModel );

    /**
     * Sets the favorite collection selection model based on which the favorite
     * collection related actions should operate. If none is set, all favorite modifications
     * actions will be disabled.
     *
     * @since 4.4
     */
    void setFavoriteSelectionModel( QItemSelectionModel *selectionModel );

    /**
     * Creates the action of the given type and adds it to the action collection
     * specified in the constructor if it does not exist yet. The action is
     * connected to its default implementation provided by this class.
     */
    KAction* createAction( Type type );

    /**
     * Convenience method to create all standard actions.
     * @see createAction()
     */
    void createAllActions();

    /**
     * Returns the action of the given type, 0 if it has not been created (yet).
     */
    KAction* action( Type type ) const;

    /**
     * Sets the label of the action @p type to @p text, which is used during
     * updating the action state and substituted according to the number of
     * selected objects. This is mainly useful to customize the label of actions
     * that can operate on multiple objects.
     *
     * Example:
     * @code
     * acctMgr->setActionText( Akonadi::StandardActionManager::CopyItems,
     *                         ki18np( "Copy Mail", "Copy %1 Mails" ) );
     * @endcode
     */
    void setActionText( Type type, const KLocalizedString &text );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the action state has been updated.
     * In case you have special needs for changing the state of some actions,
     * connect to this signal and adjust the action state.
     */
    void actionStateUpdated();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void updateActions() )
    Q_PRIVATE_SLOT( d, void clipboardChanged(QClipboard::Mode) )
    Q_PRIVATE_SLOT( d, void collectionSelectionChanged() )
    Q_PRIVATE_SLOT( d, void favoriteSelectionChanged() )

    Q_PRIVATE_SLOT( d, void slotCreateCollection() )
    Q_PRIVATE_SLOT( d, void slotCopyCollections() )
    Q_PRIVATE_SLOT( d, void slotCutCollections() )
    Q_PRIVATE_SLOT( d, void slotDeleteCollection() )
    Q_PRIVATE_SLOT( d, void slotSynchronizeCollection() )
    Q_PRIVATE_SLOT( d, void slotCollectionProperties() )
    Q_PRIVATE_SLOT( d, void slotCopyItems() )
    Q_PRIVATE_SLOT( d, void slotCutItems() )
    Q_PRIVATE_SLOT( d, void slotPaste() )
    Q_PRIVATE_SLOT( d, void slotDeleteItems() )
    Q_PRIVATE_SLOT( d, void slotLocalSubscription() )
    Q_PRIVATE_SLOT( d, void slotAddToFavorites() )
    Q_PRIVATE_SLOT( d, void slotRemoveFromFavorites() )
    Q_PRIVATE_SLOT( d, void slotRenameFavorite() )
    Q_PRIVATE_SLOT( d, void slotCopyCollectionTo(QAction*) )
    Q_PRIVATE_SLOT( d, void slotMoveCollectionTo(QAction*) )
    Q_PRIVATE_SLOT( d, void slotCopyItemTo(QAction*) )
    Q_PRIVATE_SLOT( d, void slotMoveItemTo(QAction*) )

    Q_PRIVATE_SLOT( d, void collectionCreationResult(KJob*) )
    Q_PRIVATE_SLOT( d, void collectionDeletionResult(KJob*) )
    Q_PRIVATE_SLOT( d, void pasteResult(KJob*) )
    //@endcond
};

}

#endif
