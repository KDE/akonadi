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

#include "akonadiwidgets_export.h"

#include <QtCore/QObject>

#include "collection.h"
#include "item.h"

class QAction;
class KActionCollection;
class KLocalizedString;
class QItemSelectionModel;
class QWidget;
class QMenu;

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
 * If you want to use your own implementation of the actual action operation and
 * not the default implementation, you can call interceptAction() on the action type
 * you want to handle yourself and connect the slot with your own implementation
 * to the triggered() signal of the action:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * StandardActionManager *manager = new StandardActionManager( actionCollection(), this );
 * manager->setCollectionSelectionModel( collectionView->collectionSelectionModel() );
 * manager->createAllActions();
 *
 * // disable default implementation
 * manager->interceptAction( StandardActionManager::CopyCollections );
 *
 * // connect your own implementation
 * connect( manager->action( StandardActionManager::CopyCollections ), SIGNAL(triggered(bool)),
 *          this, SLOT(myCopyImplementation()) );
 * ...
 *
 * void MyClass::myCopyImplementation()
 * {
 *   const Collection::List collections = manager->selectedCollections();
 *   foreach ( const Collection &collection, collections ) {
 *     // copy the collection manually...
 *   }
 * }
 *
 * @endcode
 *
 * @todo collection deleting and sync do not support multi-selection yet
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_EXPORT StandardActionManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Describes the supported actions.
     */
    enum Type {
        CreateCollection,                        ///< Creates an collection
        CopyCollections,                         ///< Copies the selected collections
        DeleteCollections,                       ///< Deletes the selected collections
        SynchronizeCollections,                  ///< Synchronizes collections
        CollectionProperties,                    ///< Provides collection properties
        CopyItems,                               ///< Copies the selected items
        Paste,                                   ///< Paste collections or items
        DeleteItems,                             ///< Deletes the selected items
        ManageLocalSubscriptions,                ///< Manages local subscriptions
        AddToFavoriteCollections,                ///< Add the collection to the favorite collections model @since 4.4
        RemoveFromFavoriteCollections,           ///< Remove the collection from the favorite collections model @since 4.4
        RenameFavoriteCollection,                ///< Rename the collection of the favorite collections model @since 4.4
        CopyCollectionToMenu,                    ///< Menu allowing to quickly copy a collection into another collection @since 4.4
        CopyItemToMenu,                          ///< Menu allowing to quickly copy an item into a collection @since 4.4
        MoveItemToMenu,                          ///< Menu allowing to move item into a collection @since 4.4
        MoveCollectionToMenu,                    ///< Menu allowing to move a collection into another collection @since 4.4
        CutItems,                                ///< Cuts the selected items @since 4.4
        CutCollections,                          ///< Cuts the selected collections @since 4.4
        CreateResource,                          ///< Creates a new resource @since 4.6
        DeleteResources,                         ///< Deletes the selected resources @since 4.6
        ResourceProperties,                      ///< Provides the resource properties @since 4.6
        SynchronizeResources,                    ///< Synchronizes the selected resources @since 4.6
        ToggleWorkOffline,                       ///< Toggles the work offline state of all resources @since 4.6
        CopyCollectionToDialog,                  ///< Copy a collection into another collection, select the target in a dialog @since 4.6
        MoveCollectionToDialog,                  ///< Move a collection into another collection, select the target in a dialog @since 4.6
        CopyItemToDialog,                        ///< Copy an item into a collection, select the target in a dialog @since 4.6
        MoveItemToDialog,                        ///< Move an item into a collection, select the target in a dialog @since 4.6
        SynchronizeCollectionsRecursive,         ///< Synchronizes collections in a recursive way @since 4.6
        MoveCollectionsToTrash,                  ///< Moves the selected collection to trash and marks it as deleted, needs EntityDeletedAttribute @since 4.8
        MoveItemsToTrash,                        ///< Moves the selected items to trash and marks them as deleted, needs EntityDeletedAttribute @since 4.8
        RestoreCollectionsFromTrash,             ///< Restores the selected collection from trash, needs EntityDeletedAttribute @since 4.8
        RestoreItemsFromTrash,                   ///< Restores the selected items from trash, needs EntityDeletedAttribute @since 4.8
        MoveToTrashRestoreCollection,            ///< Move Collection to Trash or Restore it from Trash, needs EntityDeletedAttribute @since 4.8
        MoveToTrashRestoreCollectionAlternative, ///< Helper type for MoveToTrashRestoreCollection, do not create directly. Use this to override texts of the restore action. @since 4.8
        MoveToTrashRestoreItem,                  ///< Move Item to Trash or Restore it from Trash, needs EntityDeletedAttribute @since 4.8
        MoveToTrashRestoreItemAlternative,       ///< Helper type for MoveToTrashRestoreItem, do not create directly. Use this to override texts of the restore action. @since 4.8
        SynchronizeFavoriteCollections,          ///< Synchronize favorite collections @since 4.8
        LastType                                 ///< Marks last action
    };

    /**
     * Describes the text context that can be customized.
     */
    enum TextContext {
        DialogTitle,                ///< The window title of a dialog
        DialogText,                 ///< The text of a dialog
        MessageBoxTitle,            ///< The window title of a message box
        MessageBoxText,             ///< The text of a message box
        MessageBoxAlternativeText,  ///< An alternative text of a message box
        ErrorMessageTitle,          ///< The window title of an error message
        ErrorMessageText            ///< The text of an error message
    };

    /**
     * Creates a new standard action manager.
     *
     * @param actionCollection The action collection to operate on.
     * @param parent The parent widget.
     */
    explicit StandardActionManager(KActionCollection *actionCollection, QWidget *parent = 0);

    /**
     * Destroys the standard action manager.
     */
    ~StandardActionManager();

    /**
     * Sets the collection selection model based on which the collection
     * related actions should operate. If none is set, all collection actions
     * will be disabled.
     *
     * @param selectionModel model to be set for collection
     */
    void setCollectionSelectionModel(QItemSelectionModel *selectionModel);

    /**
     * Sets the item selection model based on which the item related actions
     * should operate. If none is set, all item actions will be disabled.
     *
     * @param selectionModel selection model for items
     */
    void setItemSelectionModel(QItemSelectionModel *selectionModel);

    /**
     * Sets the favorite collections model based on which the collection
     * relatedactions should operate. If none is set, the "Add to Favorite Folders" action
     * will be disabled.
     *
     * @param favoritesModel model for the user's favorite collections
     * @since 4.4
     */
    void setFavoriteCollectionsModel(FavoriteCollectionsModel *favoritesModel);

    /**
     * Sets the favorite collection selection model based on which the favorite
     * collection related actions should operate. If none is set, all favorite modifications
     * actions will be disabled.
     *
     * @param selectionModel selection model for favorite collections
     * @since 4.4
     */
    void setFavoriteSelectionModel(QItemSelectionModel *selectionModel);

    /**
     * Creates the action of the given type and adds it to the action collection
     * specified in the constructor if it does not exist yet. The action is
     * connected to its default implementation provided by this class.
     *
     * @param type action to be created
     */
    QAction *createAction(Type type);

    /**
     * Convenience method to create all standard actions.
     * @see createAction()
     */
    void createAllActions();

    /**
     * Returns the action of the given type, 0 if it has not been created (yet).
     * @param type action type
     */
    QAction *action(Type type) const;

    /**
     * Sets the label of the action @p type to @p text, which is used during
     * updating the action state and substituted according to the number of
     * selected objects. This is mainly useful to customize the label of actions
     * that can operate on multiple objects.
     * @param type the action to set a text for
     * @param text the text to display for the given action
     * Example:
     * @code
     * acctMgr->setActionText( Akonadi::StandardActionManager::CopyItems,
     *                         ki18np( "Copy Mail", "Copy %1 Mails" ) );
     * @endcode
     */
    void setActionText(Type type, const KLocalizedString &text);

    /**
     * Sets whether the default implementation for the given action @p type
     * shall be executed when the action is triggered.
     *
     * @param type action type
     * @param intercept If @c false, the default implementation will be executed,
     *                  if @c true no action is taken.
     *
     * @since 4.6
     */
    void interceptAction(Type type, bool intercept = true);

    /**
     * Returns the list of collections that are currently selected.
     * The list is empty if no collection is currently selected.
     *
     * @since 4.6
     */
    Akonadi::Collection::List selectedCollections() const;

    /**
     * Returns the list of items that are currently selected.
     * The list is empty if no item is currently selected.
     *
     * @since 4.6
     */
    Akonadi::Item::List selectedItems() const;

    /**
     * Sets the @p text of the action @p type for the given @p context.
     *
     * @param type action type
     * @param context context for action
     * @param text content to set for the action
     * @since 4.6
     */
    void setContextText(Type type, TextContext context, const QString &text);

    /**
     * Sets the @p text of the action @p type for the given @p context.
     *
     * @param type action type
     * @param context context for action
     * @param text content to set for the action
     * @since 4.6
     */
    void setContextText(Type type, TextContext context, const KLocalizedString &text);

    /**
     * Sets the mime type filter that will be used when creating new resources.
     *
     * @param mimeTypes filter for creating new resources
     * @since 4.6
     */
    void setMimeTypeFilter(const QStringList &mimeTypes);

    /**
     * Sets the capability filter that will be used when creating new resources.
     *
     * @param capabilities filter for creating new resources
     * @since 4.6
     */
    void setCapabilityFilter(const QStringList &capabilities);

    /**
     * Sets the page @p names of the config pages that will be used by the
     * built-in collection properties dialog.
     *
     * @param names list of names which will be used
     * @since 4.6
     */
    void setCollectionPropertiesPageNames(const QStringList &names);

    /**
     * Create a popup menu.
     *
     * @param menu parent menu for a popup
     * @param type action type
     * @since 4.8
     */
    void createActionFolderMenu(QMenu *menu, Type type);

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
    Private *const d;

    Q_PRIVATE_SLOT(d, void updateActions())
#ifndef QT_NO_CLIPBOARD
    Q_PRIVATE_SLOT(d, void clipboardChanged(QClipboard::Mode))
#endif
    Q_PRIVATE_SLOT(d, void collectionSelectionChanged())
    Q_PRIVATE_SLOT(d, void favoriteSelectionChanged())

    Q_PRIVATE_SLOT(d, void slotCreateCollection())
    Q_PRIVATE_SLOT(d, void slotCopyCollections())
    Q_PRIVATE_SLOT(d, void slotCutCollections())
    Q_PRIVATE_SLOT(d, void slotDeleteCollection())
    Q_PRIVATE_SLOT(d, void slotMoveCollectionToTrash())
    Q_PRIVATE_SLOT(d, void slotMoveItemToTrash())
    Q_PRIVATE_SLOT(d, void slotRestoreCollectionFromTrash())
    Q_PRIVATE_SLOT(d, void slotRestoreItemFromTrash())
    Q_PRIVATE_SLOT(d, void slotTrashRestoreCollection())
    Q_PRIVATE_SLOT(d, void slotTrashRestoreItem())
    Q_PRIVATE_SLOT(d, void slotSynchronizeCollection())
    Q_PRIVATE_SLOT(d, void slotSynchronizeCollectionRecursive())
    Q_PRIVATE_SLOT(d, void slotSynchronizeFavoriteCollections())
    Q_PRIVATE_SLOT(d, void slotCollectionProperties())
    Q_PRIVATE_SLOT(d, void slotCopyItems())
    Q_PRIVATE_SLOT(d, void slotCutItems())
    Q_PRIVATE_SLOT(d, void slotPaste())
    Q_PRIVATE_SLOT(d, void slotDeleteItems())
    Q_PRIVATE_SLOT(d, void slotDeleteItemsDeferred(const Akonadi::Item::List &))
    Q_PRIVATE_SLOT(d, void slotLocalSubscription())
    Q_PRIVATE_SLOT(d, void slotAddToFavorites())
    Q_PRIVATE_SLOT(d, void slotRemoveFromFavorites())
    Q_PRIVATE_SLOT(d, void slotRenameFavorite())
    Q_PRIVATE_SLOT(d, void slotCopyCollectionTo())
    Q_PRIVATE_SLOT(d, void slotMoveCollectionTo())
    Q_PRIVATE_SLOT(d, void slotCopyItemTo())
    Q_PRIVATE_SLOT(d, void slotMoveItemTo())
    Q_PRIVATE_SLOT(d, void slotCopyCollectionTo(QAction *))
    Q_PRIVATE_SLOT(d, void slotMoveCollectionTo(QAction *))
    Q_PRIVATE_SLOT(d, void slotCopyItemTo(QAction *))
    Q_PRIVATE_SLOT(d, void slotMoveItemTo(QAction *))
    Q_PRIVATE_SLOT(d, void slotCreateResource())
    Q_PRIVATE_SLOT(d, void slotDeleteResource())
    Q_PRIVATE_SLOT(d, void slotResourceProperties())
    Q_PRIVATE_SLOT(d, void slotSynchronizeResource())
    Q_PRIVATE_SLOT(d, void slotToggleWorkOffline(bool))

    Q_PRIVATE_SLOT(d, void collectionCreationResult(KJob *))
    Q_PRIVATE_SLOT(d, void collectionDeletionResult(KJob *))
    Q_PRIVATE_SLOT(d, void moveCollectionToTrashResult(KJob *))
    Q_PRIVATE_SLOT(d, void moveItemToTrashResult(KJob *))
    Q_PRIVATE_SLOT(d, void itemDeletionResult(KJob *))
    Q_PRIVATE_SLOT(d, void resourceCreationResult(KJob *))
    Q_PRIVATE_SLOT(d, void pasteResult(KJob *))

    Q_PRIVATE_SLOT(d, void enableAction(int, bool))
    Q_PRIVATE_SLOT(d, void updatePluralLabel(int, int))
    Q_PRIVATE_SLOT(d, void updateAlternatingAction(int))
    Q_PRIVATE_SLOT(d, bool isFavoriteCollection(const Akonadi::Collection &))

    Q_PRIVATE_SLOT(d, void aboutToShowMenu())
    //@endcond
};

}

#endif
