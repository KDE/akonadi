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

#include "actionstatemanager_p.h"
#include "agentfilterproxymodel.h"
#include "agentinstancecreatejob.h"
#include "agentmanager.h"
#include "agenttypedialog.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectiondialog.h"
#include "collectionmodel.h"
#include "collectionutils_p.h"
#include "entitytreemodel.h"
#include "favoritecollectionsmodel.h"
#include "itemdeletejob.h"
#include "itemmodel.h"
#include "metatypes.h"
#include "pastehelper_p.h"
#include "specialcollectionattribute_p.h"
#include "collectionpropertiesdialog.h"
#include "subscriptiondialog_p.h"
#include "renamefavoritedialog.h"
#include "trashjob.h"
#include "trashrestorejob.h"
#include "entitydeletedattribute.h"
#include "recentcollectionaction_p.h"

#include <KIcon>
#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KDebug>
#include <KInputDialog>
#include <KLocalizedString>
#include <KMenu>
#include <KMessageBox>
#include <KToggleAction>

#include <QtCore/QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QItemSelectionModel>
#include <QPointer>
#include <QWeakPointer>

#include <boost/static_assert.hpp>

using namespace Akonadi;

//@cond PRIVATE

enum ActionType
{
    NormalAction,
    ActionWithAlternative, //Normal action, but with an alternative state
    ActionAlternative, //Alternative state of the ActionWithAlternative
    MenuAction,
    ToggleAction
};

static const struct {
    const char *name;
    const char *label;
    const char *iconLabel;
    const char *icon;
    int shortcut;
    const char *slot;
    ActionType actionType;
} standardActionData[] = {
    { "akonadi_collection_create", I18N_NOOP("&New Folder..."), I18N_NOOP("New"), "folder-new", 0, SLOT(slotCreateCollection()), NormalAction },
    { "akonadi_collection_copy", 0, 0, "edit-copy", 0, SLOT(slotCopyCollections()), NormalAction },
    { "akonadi_collection_delete", I18N_NOOP("&Delete Folder"), I18N_NOOP("Delete"), "edit-delete", 0, SLOT(slotDeleteCollection()), NormalAction },
    { "akonadi_collection_sync", I18N_NOOP("&Synchronize Folder"), I18N_NOOP("Synchronize"), "view-refresh", Qt::Key_F5, SLOT(slotSynchronizeCollection()), NormalAction },
    { "akonadi_collection_properties", I18N_NOOP("Folder &Properties"), I18N_NOOP("Properties"), "configure", 0, SLOT(slotCollectionProperties()), NormalAction },
    { "akonadi_item_copy", 0, 0, "edit-copy", 0, SLOT(slotCopyItems()), NormalAction },
    { "akonadi_paste", I18N_NOOP("&Paste"), I18N_NOOP("Paste"), "edit-paste", Qt::CTRL + Qt::Key_V, SLOT(slotPaste()), NormalAction },
    { "akonadi_item_delete", 0, 0, "edit-delete", Qt::Key_Delete, SLOT(slotDeleteItems()), NormalAction },
    { "akonadi_manage_local_subscriptions", I18N_NOOP("Manage Local &Subscriptions..."), I18N_NOOP("Manage Local Subscriptions"), "folder-bookmarks", 0, SLOT(slotLocalSubscription()), NormalAction },
    { "akonadi_collection_add_to_favorites", I18N_NOOP("Add to Favorite Folders"), I18N_NOOP("Add to Favorite"), "bookmark-new", 0, SLOT(slotAddToFavorites()), NormalAction },
    { "akonadi_collection_remove_from_favorites", I18N_NOOP("Remove from Favorite Folders"), I18N_NOOP("Remove from Favorite"), "edit-delete", 0, SLOT(slotRemoveFromFavorites()), NormalAction },
    { "akonadi_collection_rename_favorite", I18N_NOOP("Rename Favorite..."), I18N_NOOP("Rename"), "edit-rename", 0, SLOT(slotRenameFavorite()), NormalAction },
    { "akonadi_collection_copy_to_menu", I18N_NOOP("Copy Folder To..."), I18N_NOOP("Copy To"), "edit-copy", 0, SLOT(slotCopyCollectionTo(QAction*)), MenuAction },
    { "akonadi_item_copy_to_menu", I18N_NOOP("Copy Item To..."), I18N_NOOP("Copy To"), "edit-copy", 0, SLOT(slotCopyItemTo(QAction*)), MenuAction },
    { "akonadi_item_move_to_menu", I18N_NOOP("Move Item To..."), I18N_NOOP("Move To"), "go-jump", 0, SLOT(slotMoveItemTo(QAction*)), MenuAction },
    { "akonadi_collection_move_to_menu", I18N_NOOP("Move Folder To..."), I18N_NOOP("Move To"), "go-jump", 0, SLOT(slotMoveCollectionTo(QAction*)), MenuAction },
    { "akonadi_item_cut", I18N_NOOP("&Cut Item"), I18N_NOOP("Cut"), "edit-cut", Qt::CTRL + Qt::Key_X, SLOT(slotCutItems()), NormalAction },
    { "akonadi_collection_cut", I18N_NOOP("&Cut Folder"), I18N_NOOP("Cut"), "edit-cut", Qt::CTRL + Qt::Key_X, SLOT(slotCutCollections()), NormalAction },
    { "akonadi_resource_create", I18N_NOOP("Create Resource"), 0, "folder-new", 0, SLOT(slotCreateResource()), NormalAction },
    { "akonadi_resource_delete", I18N_NOOP("Delete Resource"), 0, "edit-delete", 0, SLOT(slotDeleteResource()), NormalAction },
    { "akonadi_resource_properties", I18N_NOOP("&Resource Properties"), I18N_NOOP("Properties"), "configure", 0, SLOT(slotResourceProperties()), NormalAction },
    { "akonadi_resource_synchronize", I18N_NOOP("Synchronize Resource"), I18N_NOOP("Synchronize"), "view-refresh", 0, SLOT(slotSynchronizeResource()), NormalAction },
    { "akonadi_work_offline", I18N_NOOP("Work Offline"), 0, "user-offline", 0, SLOT(slotToggleWorkOffline(bool)), ToggleAction },
    { "akonadi_collection_copy_to_dialog", I18N_NOOP("Copy Folder To..."), I18N_NOOP("Copy To"), "edit-copy", 0, SLOT(slotCopyCollectionTo()), NormalAction },
    { "akonadi_collection_move_to_dialog", I18N_NOOP("Move Folder To..."), I18N_NOOP("Move To"), "go-jump", 0, SLOT(slotMoveCollectionTo()), NormalAction },
    { "akonadi_item_copy_to_dialog", I18N_NOOP("Copy Item To..."), I18N_NOOP("Copy To"), "edit-copy", 0, SLOT(slotCopyItemTo()), NormalAction },
    { "akonadi_item_move_to_dialog", I18N_NOOP("Move Item To..."), I18N_NOOP("Move To"), "go-jump", 0, SLOT(slotMoveItemTo()), NormalAction },
    { "akonadi_collection_sync_recursive", I18N_NOOP("&Synchronize Folder Recursively"), I18N_NOOP("Synchronize Recursively"), "view-refresh", Qt::CTRL + Qt::Key_F5, SLOT(slotSynchronizeCollectionRecursive()), NormalAction },
    { "akonadi_move_collection_to_trash", I18N_NOOP("&Move Folder To Trash"), I18N_NOOP("Move Folder To Trash"), "user-trash", 0, SLOT(slotMoveCollectionToTrash()), NormalAction },
    { "akonadi_move_item_to_trash", I18N_NOOP("&Move Item To Trash"), I18N_NOOP("Move Item To Trash"), "user-trash", 0, SLOT(slotMoveItemToTrash()), NormalAction },
    { "akonadi_restore_collection_from_trash", I18N_NOOP("&Restore Folder From Trash"), I18N_NOOP("Restore Folder From Trash"), "view-refresh", 0, SLOT(slotRestoreCollectionFromTrash()), NormalAction },
    { "akonadi_restore_item_from_trash", I18N_NOOP("&Restore Item From Trash"), I18N_NOOP("Restore Item From Trash"), "view-refresh", 0, SLOT(slotRestoreItemFromTrash()), NormalAction },
    { "akonadi_collection_trash_restore", I18N_NOOP("&Restore Folder From Trash"), I18N_NOOP("Restore Folder From Trash"), "user-trash", 0, SLOT(slotTrashRestoreCollection()), ActionWithAlternative },
    { 0, I18N_NOOP("&Restore Collection From Trash"), I18N_NOOP("Restore Collection From Trash"), "view-refresh", 0, 0, ActionAlternative },
    { "akonadi_item_trash_restore", I18N_NOOP("&Restore Item From Trash"), I18N_NOOP("Restore Item From Trash"), "user-trash", 0, SLOT(slotTrashRestoreItem()), ActionWithAlternative },
    { 0, I18N_NOOP("&Restore Item From Trash"), I18N_NOOP("Restore Item From Trash"), "view-refresh", 0, 0, ActionAlternative },
    { "akonadi_collection_sync_favorite_folders", I18N_NOOP("&Synchronize Favorite Folders"), I18N_NOOP("Synchronize Favorite Folders"), "view-refresh", Qt::CTRL + Qt::SHIFT + Qt::Key_L , SLOT(slotSynchronizeFavoriteCollections()), NormalAction },
    { "akonadi_resource_synchronize_collectiontree", I18N_NOOP("Synchronize Folder Tree"), I18N_NOOP("Synchronize"), "view-refresh", 0, SLOT(slotSynchronizeCollectionTree()), NormalAction }

};
static const int numStandardActionData = sizeof standardActionData / sizeof * standardActionData;

BOOST_STATIC_ASSERT(numStandardActionData == StandardActionManager::LastType);

static bool canCreateCollection(const Akonadi::Collection &collection)
{
    if (!(collection.rights() & Akonadi::Collection::CanCreateCollection)) {
        return false;
    }

    return true;
}

/*
static inline bool isRootCollection( const Akonadi::Collection &collection )
{
  return (collection == Akonadi::Collection::root());
}
*/

static void setWorkOffline(bool offline)
{
    KConfig config(QLatin1String("akonadikderc"));
    KConfigGroup group(&config, QLatin1String("Actions"));

    group.writeEntry("WorkOffline", offline);
}

static bool workOffline()
{
    KConfig config(QLatin1String("akonadikderc"));
    const KConfigGroup group(&config, QLatin1String("Actions"));

    return group.readEntry("WorkOffline", false);
}

static QModelIndexList safeSelectedRows(QItemSelectionModel *selectionModel)
{
    QModelIndexList selectedRows = selectionModel->selectedRows();
    if (!selectedRows.isEmpty()) {
        return selectedRows;
    }

    // try harder for selected rows that don't span the full row for some reason (e.g. due to buggy column adding proxy models etc)
    foreach (const QItemSelectionRange &range, selectionModel->selection()) {
        if (!range.isValid() || range.isEmpty()) {
            continue;
        }
        const QModelIndex parent = range.parent();
        for (int row = range.top(); row <= range.bottom(); ++row) {
            const QModelIndex index = range.model()->index(row, range.left(), parent);
            const Qt::ItemFlags flags = range.model()->flags(index);
            if ((flags &Qt::ItemIsSelectable) && (flags &Qt::ItemIsEnabled)) {
                selectedRows.push_back(index);
            }
        }
    }

    return selectedRows;
}

/**
 * @internal
 */
class StandardActionManager::Private
{
public:
    Private(StandardActionManager *parent)
        : q(parent)
        , actionCollection(0)
        , parentWidget(0)
        , collectionSelectionModel(0)
        , itemSelectionModel(0)
        , favoritesModel(0)
        , favoriteSelectionModel(0)
        , insideSelectionSlot(false)
    {
        actions.fill(0, StandardActionManager::LastType);

        pluralLabels.insert(StandardActionManager::CopyCollections,
                            ki18np("&Copy Folder", "&Copy %1 Folders"));
        pluralLabels.insert(StandardActionManager::CopyItems,
                            ki18np("&Copy Item", "&Copy %1 Items"));
        pluralLabels.insert(StandardActionManager::CutItems,
                            ki18np("&Cut Item", "&Cut %1 Items"));
        pluralLabels.insert(StandardActionManager::CutCollections,
                            ki18np("&Cut Folder", "&Cut %1 Folders"));
        pluralLabels.insert(StandardActionManager::DeleteItems,
                            ki18np("&Delete Item", "&Delete %1 Items"));
        pluralLabels.insert(StandardActionManager::DeleteCollections,
                            ki18np("&Delete Folder", "&Delete %1 Folders"));
        pluralLabels.insert(StandardActionManager::SynchronizeCollections,
                            ki18np("&Synchronize Folder", "&Synchronize %1 Folders"));
        pluralLabels.insert(StandardActionManager::DeleteResources,
                            ki18np("&Delete Resource", "&Delete %1 Resources"));
        pluralLabels.insert(StandardActionManager::SynchronizeResources,
                            ki18np("&Synchronize Resource", "&Synchronize %1 Resources"));

        pluralIconLabels.insert(StandardActionManager::CopyCollections,
                                ki18np("Copy Folder", "Copy %1 Folders"));
        pluralIconLabels.insert(StandardActionManager::CopyItems,
                                ki18np("Copy Item", "Copy %1 Items"));
        pluralIconLabels.insert(StandardActionManager::CutItems,
                                ki18np("Cut Item", "Cut %1 Items"));
        pluralIconLabels.insert(StandardActionManager::CutCollections,
                                ki18np("Cut Folder", "Cut %1 Folders"));
        pluralIconLabels.insert(StandardActionManager::DeleteItems,
                                ki18np("Delete Item", "Delete %1 Items"));
        pluralIconLabels.insert(StandardActionManager::DeleteCollections,
                                ki18np("Delete Folder", "Delete %1 Folders"));
        pluralIconLabels.insert(StandardActionManager::SynchronizeCollections,
                                ki18np("Synchronize Folder", "Synchronize %1 Folders"));
        pluralIconLabels.insert(StandardActionManager::DeleteResources,
                                ki18np("Delete Resource", "Delete %1 Resources"));
        pluralIconLabels.insert(StandardActionManager::SynchronizeResources,
                                ki18np("Synchronize Resource", "Synchronize %1 Resources"));

        setContextText(StandardActionManager::CreateCollection, StandardActionManager::DialogTitle,
                       i18nc("@title:window", "New Folder"));
        setContextText(StandardActionManager::CreateCollection, StandardActionManager::DialogText,
                       i18nc("@label:textbox name of a thing", "Name"));
        setContextText(StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageText,
                       ki18n("Could not create folder: %1"));
        setContextText(StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageTitle,
                       i18n("Folder creation failed"));

        setContextText(StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxText,
                       ki18np("Do you really want to delete this folder and all its sub-folders?",
                              "Do you really want to delete %1 folders and all their sub-folders?"));
        setContextText(StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxTitle,
                       ki18ncp("@title:window", "Delete folder?", "Delete folders?"));
        setContextText(StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageText,
                       ki18n("Could not delete folder: %1"));
        setContextText(StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageTitle,
                       i18n("Folder deletion failed"));

        setContextText(StandardActionManager::CollectionProperties, StandardActionManager::DialogTitle,
                       ki18nc("@title:window", "Properties of Folder %1"));

        setContextText(StandardActionManager::DeleteItems, StandardActionManager::MessageBoxText,
                       ki18np("Do you really want to delete the selected item?",
                              "Do you really want to delete %1 items?"));
        setContextText(StandardActionManager::DeleteItems, StandardActionManager::MessageBoxTitle,
                       ki18ncp("@title:window", "Delete item?", "Delete items?"));
        setContextText(StandardActionManager::DeleteItems, StandardActionManager::ErrorMessageText,
                       ki18n("Could not delete item: %1"));
        setContextText(StandardActionManager::DeleteItems, StandardActionManager::ErrorMessageTitle,
                       i18n("Item deletion failed"));

        setContextText(StandardActionManager::RenameFavoriteCollection, StandardActionManager::DialogTitle,
                       i18nc("@title:window", "Rename Favorite"));
        setContextText(StandardActionManager::RenameFavoriteCollection, StandardActionManager::DialogText,
                       i18nc("@label:textbox name of the folder", "Name:"));

        setContextText(StandardActionManager::CreateResource, StandardActionManager::DialogTitle,
                       i18nc("@title:window", "New Resource"));
        setContextText(StandardActionManager::CreateResource, StandardActionManager::ErrorMessageText,
                       ki18n("Could not create resource: %1"));
        setContextText(StandardActionManager::CreateResource, StandardActionManager::ErrorMessageTitle,
                       i18n("Resource creation failed"));

        setContextText(StandardActionManager::DeleteResources, StandardActionManager::MessageBoxText,
                       ki18np("Do you really want to delete this resource?",
                              "Do you really want to delete %1 resources?"));
        setContextText(StandardActionManager::DeleteResources, StandardActionManager::MessageBoxTitle,
                       ki18ncp("@title:window", "Delete Resource?", "Delete Resources?"));

        setContextText(StandardActionManager::Paste, StandardActionManager::ErrorMessageText,
                       ki18n("Could not paste data: %1"));
        setContextText(StandardActionManager::Paste, StandardActionManager::ErrorMessageTitle,
                       i18n("Paste failed"));

        qRegisterMetaType<Akonadi::Item::List>("Akonadi::Item::List");
    }
    void enableAction(int type, bool enable)
    {
        enableAction(static_cast<StandardActionManager::Type>(type), enable);
    }

    void enableAction(StandardActionManager::Type type, bool enable)
    {
        Q_ASSERT(type < StandardActionManager::LastType);
        if (actions[type]) {
            actions[type]->setEnabled(enable);
        }

        // Update the action menu
        KActionMenu *actionMenu = qobject_cast<KActionMenu *>(actions[type]);
        if (actionMenu) {
            //get rid of the submenus, they are re-created in enableAction. clear() is not enough, doesn't remove the submenu object instances.
            KMenu *menu = actionMenu->menu();
            //Not necessary to delete and recreate menu when it was not created
            if (menu->property("actionType").isValid() && menu->isEmpty()) {
                return;
            }
            mRecentCollectionsMenu.remove(type);
            delete menu;
            menu = new KMenu();

            menu->setProperty("actionType", static_cast<int>(type));
            q->connect(menu, SIGNAL(aboutToShow()), SLOT(aboutToShowMenu()));
            q->connect(menu, SIGNAL(triggered(QAction*)), standardActionData[type].slot);
            actionMenu->setMenu(menu);
        }
    }

    void aboutToShowMenu()
    {
        QMenu *menu = qobject_cast<QMenu *>(q->sender());
        if (!menu) {
            return;
        }

        if (!menu->isEmpty()) {
            return;
        }
        // collect all selected collections
        const Akonadi::Collection::List selectedCollectionsList = selectedCollections();
        const StandardActionManager::Type type = static_cast<StandardActionManager::Type>(menu->property("actionType").toInt());

        QWeakPointer<RecentCollectionAction> recentCollection = new RecentCollectionAction(collectionSelectionModel->model(), menu);
        mRecentCollectionsMenu.insert(type, recentCollection);
        const QSet<QString> mimeTypes = mimeTypesOfSelection(type);
        fillFoldersMenu(selectedCollectionsList,
                        mimeTypes,
                        type,
                        menu,
                        collectionSelectionModel->model(),
                        QModelIndex());
    }

    void createActionFolderMenu(QMenu *menu, StandardActionManager::Type type)
    {
        if (type == CopyCollectionToMenu ||
            type == CopyItemToMenu ||
            type == MoveItemToMenu ||
            type == MoveCollectionToMenu)
        {

            QWeakPointer<RecentCollectionAction> recentCollection = new RecentCollectionAction(collectionSelectionModel->model(), menu);
            Collection::List selectedCollectionsList = selectedCollections();
            const QSet<QString> mimeTypes = mimeTypesOfSelection(type);
            fillFoldersMenu(selectedCollectionsList,
                            mimeTypes,
                            type,
                            menu,
                            collectionSelectionModel->model(),
                            QModelIndex());
        }
    }

    void updateAlternatingAction(int type)
    {
        updateAlternatingAction(static_cast<StandardActionManager::Type>(type));
    }

    void updateAlternatingAction(StandardActionManager::Type type)
    {
        Q_ASSERT(type < StandardActionManager::LastType);
        if (!actions[type]) {
            return;
        }

        /*
         * The same action is stored at the ActionWithAlternative indexes as well as the corresponding ActionAlternative indexes in the actions array.
         * The following simply changes the standardActionData
         */
        if ((standardActionData[type].actionType == ActionWithAlternative) || (standardActionData[type].actionType == ActionAlternative)) {
            actions[type]->setText(i18n(standardActionData[type].label));
            actions[type]->setIcon(KIcon(QString::fromLatin1(standardActionData[type].icon)));

            if (pluralLabels.contains(type) && !pluralLabels.value(type).isEmpty()) {
                actions[type]->setText(pluralLabels.value(type).subs(1).toString());
            } else if (standardActionData[type].label) {
                actions[type]->setText(i18n(standardActionData[type].label));
            }

            if (pluralIconLabels.contains(type) && !pluralIconLabels.value(type).isEmpty()) {
                actions[type]->setIconText(pluralIconLabels.value(type).subs(1).toString());
            } else if (standardActionData[type].iconLabel) {
                actions[type]->setIconText(i18n(standardActionData[type].iconLabel));
            }

            if (standardActionData[type].icon) {
                actions[type]->setIcon(KIcon(QString::fromLatin1(standardActionData[type].icon)));
            }

            //actions[type]->setShortcut( standardActionData[type].shortcut );

            /*if ( standardActionData[type].slot ) {
              switch ( standardActionData[type].actionType ) {
              case NormalAction:
              case ActionWithAlternative:
                  connect( action, SIGNAL(triggered()), standardActionData[type].slot );
                  break;
              }
            }*/
        }
    }

    void updatePluralLabel(int type, int count)
    {
        updatePluralLabel(static_cast<StandardActionManager::Type>(type), count);
    }

    void updatePluralLabel(StandardActionManager::Type type, int count)
    {
        Q_ASSERT(type < StandardActionManager::LastType);
        if (actions[type] && pluralLabels.contains(type) && !pluralLabels.value(type).isEmpty()) {
            actions[type]->setText(pluralLabels.value(type).subs(qMax(count, 1)).toString());
        }
    }

    bool isFavoriteCollection(const Akonadi::Collection &collection)
    {
        if (!favoritesModel) {
            return false;
        }

        return favoritesModel->collectionIds().contains(collection.id());
    }

    void encodeToClipboard(QItemSelectionModel *selectionModel, bool cut = false)
    {
        Q_ASSERT(selectionModel);
        if (safeSelectedRows(selectionModel).count() <= 0) {
            return;
        }

#ifndef QT_NO_CLIPBOARD
        QMimeData *mimeData = selectionModel->model()->mimeData(safeSelectedRows(selectionModel));
        markCutAction(mimeData, cut);
        QApplication::clipboard()->setMimeData(mimeData);

        QAbstractItemModel *model = const_cast<QAbstractItemModel *>(selectionModel->model());

        foreach (const QModelIndex &index, safeSelectedRows(selectionModel)) {
            model->setData(index, true, EntityTreeModel::PendingCutRole);
        }
#endif
    }

    void updateActions()
    {
        // collect all selected collections
        Collection::List selectedCollectionsList;
        if (collectionSelectionModel) {
            const QModelIndexList rows = safeSelectedRows(collectionSelectionModel);
            foreach (const QModelIndex &index, rows) {
                Collection collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
                if (!collection.isValid()) {
                    continue;
                }

                const Collection parentCollection = index.data(EntityTreeModel::ParentCollectionRole).value<Collection>();
                collection.setParentCollection(parentCollection);

                selectedCollectionsList << collection;
            }
        }

        // collect all selected items
        Item::List selectedItems;
        if (itemSelectionModel) {
            const QModelIndexList rows = safeSelectedRows(itemSelectionModel);
            foreach (const QModelIndex &index, rows) {
                Item item = index.data(EntityTreeModel::ItemRole).value<Item>();
                if (!item.isValid()) {
                    continue;
                }

                const Collection parentCollection = index.data(EntityTreeModel::ParentCollectionRole).value<Collection>();
                item.setParentCollection(parentCollection);

                selectedItems << item;
            }
        }

        mActionStateManager.updateState(selectedCollectionsList, selectedItems);
        if (favoritesModel) {
            enableAction(StandardActionManager::SynchronizeFavoriteCollections, (favoritesModel->rowCount() > 0));
        }
        emit q->actionStateUpdated();
    }

#ifndef QT_NO_CLIPBOARD
    void clipboardChanged(QClipboard::Mode mode)
    {
        if (mode == QClipboard::Clipboard) {
            updateActions();
        }
    }
#endif

    QItemSelection mapToEntityTreeModel(const QAbstractItemModel *model, const QItemSelection &selection) const
    {
        const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel *>(model);
        if (proxy) {
            return mapToEntityTreeModel(proxy->sourceModel(), proxy->mapSelectionToSource(selection));
        } else {
            return selection;
        }
    }

    QItemSelection mapFromEntityTreeModel(const QAbstractItemModel *model, const QItemSelection &selection) const
    {
        const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel *>(model);
        if (proxy) {
            const QItemSelection select = mapFromEntityTreeModel(proxy->sourceModel(), selection);
            return proxy->mapSelectionFromSource(select);
        } else {
            return selection;
        }
    }

    // RAII class for setting insideSelectionSlot to true on entering, and false on exiting, the two slots below.
    class InsideSelectionSlotBlocker {
    public:
        InsideSelectionSlotBlocker(Private *p)
            : _p(p)
        {
            Q_ASSERT(!p->insideSelectionSlot);
            p->insideSelectionSlot = true;
        }

        ~InsideSelectionSlotBlocker()
        {
            Q_ASSERT(_p->insideSelectionSlot);
            _p->insideSelectionSlot = false;
        }
    private:
        Private *_p;
    };

    void collectionSelectionChanged()
    {
        if (insideSelectionSlot) {
            return;
        }
        InsideSelectionSlotBlocker block(this);
        QItemSelection selection = collectionSelectionModel->selection();
        selection = mapToEntityTreeModel(collectionSelectionModel->model(), selection);
        selection = mapFromEntityTreeModel(favoritesModel, selection);

        if (favoriteSelectionModel) {
            favoriteSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
        }

        updateActions();
    }

    void favoriteSelectionChanged()
    {
        if (insideSelectionSlot) {
            return;
        }
        QItemSelection selection = favoriteSelectionModel->selection();
        if (selection.isEmpty()) {
            return;
        }

        selection = mapToEntityTreeModel(favoritesModel, selection);
        selection = mapFromEntityTreeModel(collectionSelectionModel->model(), selection);

        InsideSelectionSlotBlocker block(this);
        collectionSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        // Also set the current index. This will trigger KMMainWidget::slotFolderChanged in kmail, which we want.
        collectionSelectionModel->setCurrentIndex(selection.indexes().first(), QItemSelectionModel::NoUpdate);

        updateActions();
    }

    void slotCreateCollection()
    {
        Q_ASSERT(collectionSelectionModel);
        if (collectionSelectionModel->selection().indexes().isEmpty()) {
            return;
        }

        const QModelIndex index = collectionSelectionModel->selection().indexes().at(0);
        Q_ASSERT(index.isValid());
        const Collection parentCollection = index.data(CollectionModel::CollectionRole).value<Collection>();
        Q_ASSERT(parentCollection.isValid());

        if (!canCreateCollection(parentCollection)) {
            return;
        }

        QString name = KInputDialog::getText(contextText(StandardActionManager::CreateCollection, StandardActionManager::DialogTitle),
                                             contextText(StandardActionManager::CreateCollection, StandardActionManager::DialogText),
                                             QString(), 0, parentWidget);
        name = name.trimmed();
        if (name.isEmpty()) {
            return;
        }

        if (name.contains(QLatin1Char('/'))) {
            KMessageBox::error(parentWidget,
                               i18n("We can not add \"/\" in folder name."),
                               i18n("Create new folder error"));
            return;
        }
        if (name.startsWith(QLatin1Char('.')) ||
            name.endsWith(QLatin1Char('.'))) {
            KMessageBox::error(parentWidget,
                               i18n("We can not add \".\" at begin or end of folder name."),
                               i18n("Create new folder error"));
            return;
        }

        Collection collection;
        collection.setName(name);
        collection.setParentCollection(parentCollection);
        if (actions[StandardActionManager::CreateCollection]) {
            const QStringList mts = actions[StandardActionManager::CreateCollection]->property("ContentMimeTypes").toStringList();
            if (!mts.isEmpty()) {
                collection.setContentMimeTypes(mts);
            }
        }
        if (parentCollection.contentMimeTypes().contains(Collection::virtualMimeType())) {
            collection.setVirtual(true);
            collection.setContentMimeTypes(collection.contentMimeTypes()
                                           << Collection::virtualMimeType());
        }

        CollectionCreateJob *job = new CollectionCreateJob(collection);
        q->connect(job, SIGNAL(result(KJob*)), q, SLOT(collectionCreationResult(KJob*)));
    }

    void slotCopyCollections()
    {
        encodeToClipboard(collectionSelectionModel);
    }

    void slotCutCollections()
    {
        encodeToClipboard(collectionSelectionModel, true);
    }

    Collection::List selectedCollections()
    {
        Collection::List collections;

        Q_ASSERT(collectionSelectionModel);

        foreach (const QModelIndex &index, safeSelectedRows(collectionSelectionModel)) {
            Q_ASSERT(index.isValid());
            const Collection collection = index.data(CollectionModel::CollectionRole).value<Collection>();
            Q_ASSERT(collection.isValid());

            collections << collection;
        }

        return collections;
    }

    void slotDeleteCollection()
    {
        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        const QString collectionName = collections.first().name();
        const QString text = contextText(StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxText,
                                         collections.count(), collectionName);

        if (KMessageBox::questionYesNo(parentWidget, text,
                                       contextText(StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxTitle, collections.count(), collectionName),
                                       KStandardGuiItem::del(), KStandardGuiItem::cancel(),
                                       QString(), KMessageBox::Dangerous) != KMessageBox::Yes) {
            return;
        }

        foreach (const Collection &collection, collections) {
            CollectionDeleteJob *job = new CollectionDeleteJob(collection, q);
            q->connect(job, SIGNAL(result(KJob*)), q, SLOT(collectionDeletionResult(KJob*)));
        }
    }

    void slotMoveCollectionToTrash()
    {
        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        foreach (const Collection &collection, collections) {
            TrashJob *job = new TrashJob(collection, q);
            q->connect(job, SIGNAL(result(KJob*)), q, SLOT(moveCollectionToTrashResult(KJob*)));
        }
    }

    void slotRestoreCollectionFromTrash()
    {
        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        foreach (const Collection &collection, collections) {
            TrashRestoreJob *job = new TrashRestoreJob(collection, q);
            q->connect(job, SIGNAL(result(KJob*)), q, SLOT(moveCollectionToTrashResult(KJob*)));
        }
    }

    Item::List selectedItems() const
    {
        Item::List items;

        Q_ASSERT(itemSelectionModel);

        foreach (const QModelIndex &index, safeSelectedRows(itemSelectionModel)) {
            Q_ASSERT(index.isValid());
            const Item item = index.data(ItemModel::ItemRole).value<Item>();
            Q_ASSERT(item.isValid());

            items << item;
        }

        return items;
    }

    void slotMoveItemToTrash()
    {
        const Item::List items = selectedItems();
        if (items.isEmpty()) {
            return;
        }

        TrashJob *job = new TrashJob(items, q);
        q->connect(job, SIGNAL(result(KJob*)), q, SLOT(moveItemToTrashResult(KJob*)));
    }

    void slotRestoreItemFromTrash()
    {
        const Item::List items = selectedItems();
        if (items.isEmpty()) {
            return;
        }

        TrashRestoreJob *job = new TrashRestoreJob(items, q);
        q->connect(job, SIGNAL(result(KJob*)), q, SLOT(moveItemToTrashResult(KJob*)));
    }

    void slotTrashRestoreCollection()
    {
        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        bool collectionsAreInTrash = false;
        foreach (const Collection &collection, collections) {
            if (collection.hasAttribute<EntityDeletedAttribute>()) {
                collectionsAreInTrash = true;
                break;
            }
        }

        if (collectionsAreInTrash) {
            slotRestoreCollectionFromTrash();
        } else {
            slotMoveCollectionToTrash();
        }
    }

    void slotTrashRestoreItem()
    {
        const Item::List items = selectedItems();
        if (items.isEmpty()) {
            return;
        }

        bool itemsAreInTrash = false;
        foreach (const Item &item, items) {
            if (item.hasAttribute<EntityDeletedAttribute>()) {
                itemsAreInTrash = true;
                break;
            }
        }

        if (itemsAreInTrash) {
            slotRestoreItemFromTrash();
        } else {
            slotMoveItemToTrash();
        }
    }

    void slotSynchronizeCollection()
    {
        Q_ASSERT(collectionSelectionModel);
        const QModelIndexList list = safeSelectedRows(collectionSelectionModel);
        if (list.isEmpty()) {
            return;
        }

        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        foreach (const Collection &collection, collections) {
            if (!testAndSetOnlineResources(collection)) {
                break;
            }
            AgentManager::self()->synchronizeCollection(collection, false);
        }
    }

    bool testAndSetOnlineResources(const Akonadi::Collection &collection)
    {
        // Shortcut for the Search resource, which is a virtual resource and thus
        // is awlays online (but AgentManager does not know about it, so it returns
        // an invalid AgentInstance, which is "offline").
        //
        // FIXME: AgentManager should return a valid AgentInstance even
        // for virtual resources, which would be always online.
        if (collection.resource() == QLatin1String("akonadi_search_resource")) {
            return true;
        }

        Akonadi::AgentInstance instance = Akonadi::AgentManager::self()->instance(collection.resource());
        if (!instance.isOnline()) {
            if (KMessageBox::questionYesNo(parentWidget,
                                           i18n("Before syncing folder \"%1\" it is necessary to have the resource online. Do you want to make it online?", collection.displayName()),
                                           i18n("Account \"%1\" is offline", instance.name()),
                                           KGuiItem(i18nc("@action:button", "Go Online")), KStandardGuiItem::cancel()) != KMessageBox::Yes) {
                return false;
            }
            instance.setIsOnline(true);
        }
        return true;
    }

    void slotSynchronizeCollectionRecursive()
    {
        Q_ASSERT(collectionSelectionModel);
        const QModelIndexList list = safeSelectedRows(collectionSelectionModel);
        if (list.isEmpty()) {
            return;
        }

        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        foreach (const Collection &collection, collections) {
            if (!testAndSetOnlineResources(collection)) {
                break;
            }
            AgentManager::self()->synchronizeCollection(collection, true);
        }
    }

    void slotCollectionProperties()
    {
        const QModelIndexList list = safeSelectedRows(collectionSelectionModel);
        if (list.isEmpty()) {
            return;
        }

        const QModelIndex index = list.first();
        Q_ASSERT(index.isValid());

        const Collection collection = index.data(CollectionModel::CollectionRole).value<Collection>();
        Q_ASSERT(collection.isValid());

        CollectionPropertiesDialog *dlg = new CollectionPropertiesDialog(collection, mCollectionPropertiesPageNames, parentWidget);
        dlg->setCaption(contextText(StandardActionManager::CollectionProperties, StandardActionManager::DialogTitle, collection.displayName()));
        dlg->show();
    }

    void slotCopyItems()
    {
        encodeToClipboard(itemSelectionModel);
    }

    void slotCutItems()
    {
        encodeToClipboard(itemSelectionModel, true);
    }

    void slotPaste()
    {
        Q_ASSERT(collectionSelectionModel);

        const QModelIndexList list = safeSelectedRows(collectionSelectionModel);
        if (list.isEmpty()) {
            return;
        }

        const QModelIndex index = list.first();
        Q_ASSERT(index.isValid());

#ifndef QT_NO_CLIPBOARD
        // TODO: Copy or move? We can't seem to cut yet
        QAbstractItemModel *model = const_cast<QAbstractItemModel *>(collectionSelectionModel->model());
        const QMimeData *mimeData = QApplication::clipboard()->mimeData();
        model->dropMimeData(mimeData, isCutAction(mimeData) ? Qt::MoveAction : Qt::CopyAction, -1, -1, index);
        model->setData(QModelIndex(), false, EntityTreeModel::PendingCutRole);
        QApplication::clipboard()->clear();
#endif
    }

    void slotDeleteItems()
    {
        Q_ASSERT(itemSelectionModel);

        Item::List items;
        foreach (const QModelIndex &index, safeSelectedRows(itemSelectionModel)) {
            bool ok;
            const qlonglong id = index.data(ItemModel::IdRole).toLongLong(&ok);
            Q_ASSERT(ok);
            items << Item(id);
        }

        if (items.isEmpty()) {
            return;
        }

        QMetaObject::invokeMethod(q, "slotDeleteItemsDeferred",
                                  Qt::QueuedConnection,
                                  Q_ARG(Akonadi::Item::List, items));
    }

    void slotDeleteItemsDeferred(const Akonadi::Item::List &items)
    {
        Q_ASSERT(itemSelectionModel);

        if (KMessageBox::questionYesNo(parentWidget,
                                       contextText(StandardActionManager::DeleteItems, StandardActionManager::MessageBoxText, items.count(), QString()),
                                       contextText(StandardActionManager::DeleteItems, StandardActionManager::MessageBoxTitle, items.count(), QString()),
                                       KStandardGuiItem::del(), KStandardGuiItem::cancel(),
                                       QString(), KMessageBox::Dangerous) != KMessageBox::Yes) {
            return;
        }

        ItemDeleteJob *job = new ItemDeleteJob(items, q);
        q->connect(job, SIGNAL(result(KJob*)), q, SLOT(itemDeletionResult(KJob*)));
    }

    void slotLocalSubscription()
    {
        SubscriptionDialog *dlg = new SubscriptionDialog(mMimeTypeFilter, parentWidget);
        dlg->showHiddenCollection(true);
        dlg->show();
    }

    void slotAddToFavorites()
    {
        Q_ASSERT(collectionSelectionModel);
        Q_ASSERT(favoritesModel);
        const QModelIndexList list = safeSelectedRows(collectionSelectionModel);
        if (list.isEmpty()) {
            return;
        }

        foreach (const QModelIndex &index, list) {
            Q_ASSERT(index.isValid());
            const Collection collection = index.data(CollectionModel::CollectionRole).value<Collection>();
            Q_ASSERT(collection.isValid());

            favoritesModel->addCollection(collection);
        }

        updateActions();
    }

    void slotRemoveFromFavorites()
    {
        Q_ASSERT(collectionSelectionModel);
        Q_ASSERT(favoritesModel);
        const QModelIndexList list = safeSelectedRows(collectionSelectionModel);
        if (list.isEmpty()) {
            return;
        }

        foreach (const QModelIndex &index, list) {
            Q_ASSERT(index.isValid());
            const Collection collection = index.data(CollectionModel::CollectionRole).value<Collection>();
            Q_ASSERT(collection.isValid());

            favoritesModel->removeCollection(collection);
        }

        updateActions();
    }

    void slotRenameFavorite()
    {
        Q_ASSERT(collectionSelectionModel);
        Q_ASSERT(favoritesModel);
        const QModelIndexList list = safeSelectedRows(collectionSelectionModel);
        if (list.isEmpty()) {
            return;
        }
        const QModelIndex index = list.first();
        Q_ASSERT(index.isValid());
        const Collection collection = index.data(CollectionModel::CollectionRole).value<Collection>();
        Q_ASSERT(collection.isValid());

        QPointer<RenameFavoriteDialog> dlg(new RenameFavoriteDialog(contextText(StandardActionManager::RenameFavoriteCollection, StandardActionManager::DialogTitle), contextText(StandardActionManager::RenameFavoriteCollection, StandardActionManager::DialogText) , favoritesModel->favoriteLabel(collection), collection.displayName(), parentWidget));
        if (dlg->exec() == QDialog::Accepted && dlg != 0) {
            favoritesModel->setFavoriteLabel(collection, dlg->newName());
        }
        delete dlg;
    }

    void slotSynchronizeFavoriteCollections()
    {
        Q_ASSERT(favoritesModel);
        foreach (const Collection &collection, favoritesModel->collections()) {
            // there might be virtual collections in favorites which cannot be checked
            // so let's be safe here, agentmanager asserts otherwise
            if (!collection.resource().isEmpty()) {
                AgentManager::self()->synchronizeCollection(collection, false);
            }
        }
    }

    void slotCopyCollectionTo()
    {
        pasteTo(collectionSelectionModel, collectionSelectionModel->model(), CopyCollectionToMenu, Qt::CopyAction);
    }

    void slotCopyItemTo()
    {
        pasteTo(itemSelectionModel, collectionSelectionModel->model(), CopyItemToMenu, Qt::CopyAction);
    }

    void slotMoveCollectionTo()
    {
        pasteTo(collectionSelectionModel, collectionSelectionModel->model(), MoveCollectionToMenu, Qt::MoveAction);
    }

    void slotMoveItemTo()
    {
        pasteTo(itemSelectionModel, collectionSelectionModel->model(), MoveItemToMenu, Qt::MoveAction);
    }

    void slotCopyCollectionTo(QAction *action)
    {
        pasteTo(collectionSelectionModel, action, Qt::CopyAction);
    }

    void slotCopyItemTo(QAction *action)
    {
        pasteTo(itemSelectionModel, action, Qt::CopyAction);
    }

    void slotMoveCollectionTo(QAction *action)
    {
        pasteTo(collectionSelectionModel, action, Qt::MoveAction);
    }

    void slotMoveItemTo(QAction *action)
    {
        pasteTo(itemSelectionModel, action, Qt::MoveAction);
    }

    AgentInstance::List selectedAgentInstances() const
    {
        AgentInstance::List instances;

        Q_ASSERT(collectionSelectionModel);
        if (collectionSelectionModel->selection().indexes().isEmpty()) {
            return instances;
        }

        foreach (const QModelIndex &index, collectionSelectionModel->selection().indexes()) {
            Q_ASSERT(index.isValid());
            const Collection collection = index.data(CollectionModel::CollectionRole).value<Collection>();
            Q_ASSERT(collection.isValid());

            if (collection.isValid()) {
                const QString identifier = collection.resource();
                instances << AgentManager::self()->instance(identifier);
            }
        }

        return instances;
    }

    AgentInstance selectedAgentInstance() const
    {
        const AgentInstance::List instances = selectedAgentInstances();

        if (instances.isEmpty()) {
            return AgentInstance();
        }

        return instances.first();
    }

    void slotCreateResource()
    {
        QPointer<Akonadi::AgentTypeDialog> dlg(new Akonadi::AgentTypeDialog(parentWidget));
        dlg->setCaption(contextText(StandardActionManager::CreateResource, StandardActionManager::DialogTitle));

        foreach (const QString &mimeType, mMimeTypeFilter) {
            dlg->agentFilterProxyModel()->addMimeTypeFilter(mimeType);
        }

        foreach (const QString &capability, mCapabilityFilter) {
            dlg->agentFilterProxyModel()->addCapabilityFilter(capability);
        }

        if (dlg->exec() == QDialog::Accepted && dlg != 0) {
            const AgentType agentType = dlg->agentType();

            if (agentType.isValid()) {
                AgentInstanceCreateJob *job = new AgentInstanceCreateJob(agentType, q);
                q->connect(job, SIGNAL(result(KJob*)), SLOT(resourceCreationResult(KJob*)));
                job->configure(parentWidget);
                job->start();
            }
        }
        delete dlg;
    }

    void slotDeleteResource()
    {
        const AgentInstance::List instances = selectedAgentInstances();
        if (instances.isEmpty()) {
            return;
        }

        if (KMessageBox::questionYesNo(parentWidget,
                                       contextText(StandardActionManager::DeleteResources, StandardActionManager::MessageBoxText, instances.count(), instances.first().name()),
                                       contextText(StandardActionManager::DeleteResources, StandardActionManager::MessageBoxTitle, instances.count(), instances.first().name()),
                                       KStandardGuiItem::del(), KStandardGuiItem::cancel(),
                                       QString(), KMessageBox::Dangerous) != KMessageBox::Yes) {
            return;
        }

        foreach (const AgentInstance &instance, instances) {
            AgentManager::self()->removeInstance(instance);
        }
    }

    void slotSynchronizeResource()
    {
        const AgentInstance::List instances = selectedAgentInstances();
        if (instances.isEmpty()) {
            return;
        }

        foreach (AgentInstance instance, instances) {    //krazy:exclude=foreach
            instance.synchronize();
        }
    }

    void slotSynchronizeCollectionTree()
    {
        const AgentInstance::List instances = selectedAgentInstances();
        if (instances.isEmpty()) {
            return;
        }

        foreach (AgentInstance instance, instances) {    //krazy:exclude=foreach
            instance.synchronizeCollectionTree();
        }
    }

    void slotResourceProperties()
    {
        AgentInstance instance = selectedAgentInstance();
        if (!instance.isValid()) {
            return;
        }

        instance.configure(parentWidget);
    }

    void slotToggleWorkOffline(bool offline)
    {
        setWorkOffline(offline);

        AgentInstance::List instances = AgentManager::self()->instances();
        foreach (AgentInstance instance, instances) {    //krazy:exclude=foreach
            instance.setIsOnline(!offline);
        }
    }

    void pasteTo(QItemSelectionModel *selectionModel, const QAbstractItemModel *model, StandardActionManager::Type type, Qt::DropAction dropAction)
    {
        const QSet<QString> mimeTypes = mimeTypesOfSelection(type);

        QPointer<CollectionDialog> dlg(new CollectionDialog(const_cast<QAbstractItemModel *>(model)));
        dlg->setMimeTypeFilter(mimeTypes.toList());

        if (type == CopyItemToMenu || type == MoveItemToMenu) {
            dlg->setAccessRightsFilter(Collection::CanCreateItem);
        } else if (type == CopyCollectionToMenu || type == MoveCollectionToMenu) {
            dlg->setAccessRightsFilter(Collection::CanCreateCollection);
        }

        if (dlg->exec() == QDialog::Accepted && dlg != 0) {
            const QModelIndex index = EntityTreeModel::modelIndexForCollection(collectionSelectionModel->model(), dlg->selectedCollection());
            if (!index.isValid()) {
                return;
            }

            const QMimeData *mimeData = selectionModel->model()->mimeData(safeSelectedRows(selectionModel));

            QAbstractItemModel *model = const_cast<QAbstractItemModel *>(index.model());
            model->dropMimeData(mimeData, dropAction, -1, -1, index);
        }
        delete dlg;
    }

    void pasteTo(QItemSelectionModel *selectionModel, QAction *action, Qt::DropAction dropAction)
    {
        Q_ASSERT(selectionModel);
        Q_ASSERT(action);

        if (safeSelectedRows(selectionModel).count() <= 0) {
            return;
        }

        const QMimeData *mimeData = selectionModel->model()->mimeData(selectionModel->selectedRows());

        const QModelIndex index = action->data().value<QModelIndex>();
        Q_ASSERT(index.isValid());

        QAbstractItemModel *model = const_cast<QAbstractItemModel *>(index.model());
        const Collection collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        addRecentCollection(collection.id());
        model->dropMimeData(mimeData, dropAction, -1, -1, index);
    }

    void addRecentCollection(Akonadi::Collection::Id id)
    {
        QMapIterator<StandardActionManager::Type, QWeakPointer<RecentCollectionAction> > item(mRecentCollectionsMenu);
        while (item.hasNext()) {
            item.next();
            if (item.value().data()) {
                item.value().data()->addRecentCollection(id);
            }
        }
    }

    void collectionCreationResult(KJob *job)
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageTitle));
        }
    }

    void collectionDeletionResult(KJob *job)
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageTitle));
        }
    }

    void moveCollectionToTrashResult(KJob *job)
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::MoveCollectionsToTrash, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::MoveCollectionsToTrash, StandardActionManager::ErrorMessageTitle));
        }
    }

    void moveItemToTrashResult(KJob *job)
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::MoveItemsToTrash, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::MoveItemsToTrash, StandardActionManager::ErrorMessageTitle));
        }
    }

    void itemDeletionResult(KJob *job)
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::DeleteItems, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::DeleteItems, StandardActionManager::ErrorMessageTitle));
        }
    }

    void resourceCreationResult(KJob *job)
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::CreateResource, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::CreateResource, StandardActionManager::ErrorMessageTitle));
        }
    }

    void pasteResult(KJob *job)
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::Paste, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::Paste, StandardActionManager::ErrorMessageTitle));
        }
    }

    /**
     * Returns a set of mime types of the entities that are currently selected.
     */
    QSet<QString> mimeTypesOfSelection(StandardActionManager::Type type) const
    {
        QModelIndexList list;
        QSet<QString> mimeTypes;

        const bool isItemAction = (type == CopyItemToMenu || type == MoveItemToMenu);
        const bool isCollectionAction = (type == CopyCollectionToMenu || type == MoveCollectionToMenu);

        if (isItemAction) {
            list = safeSelectedRows(itemSelectionModel);
            foreach (const QModelIndex &index, list) {
                mimeTypes << index.data(EntityTreeModel::MimeTypeRole).toString();
            }
        }

        if (isCollectionAction) {
            list = safeSelectedRows(collectionSelectionModel);
            foreach (const QModelIndex &index, list) {
                const Collection collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();

                // The mimetypes that the selected collection can possibly contain
                mimeTypes = AgentManager::self()->instance(collection.resource()).type().mimeTypes().toSet();
            }
        }

        return mimeTypes;
    }

    /**
     * Returns whether items with the given @p mimeTypes can be written to the given @p collection.
     */
    bool isWritableTargetCollectionForMimeTypes(const Collection &collection, const QSet<QString> &mimeTypes, StandardActionManager::Type type) const
    {
        if (collection.isVirtual()) {
            return false;
        }

        const bool isItemAction = (type == CopyItemToMenu || type == MoveItemToMenu);
        const bool isCollectionAction = (type == CopyCollectionToMenu || type == MoveCollectionToMenu);

        const bool canContainRequiredMimeTypes = !collection.contentMimeTypes().toSet().intersect(mimeTypes).isEmpty();
        const bool canCreateNewItems = (collection.rights() & Collection::CanCreateItem);

        const bool canCreateNewCollections = (collection.rights() & Collection::CanCreateCollection);
        const bool canContainCollections = collection.contentMimeTypes().contains(Collection::mimeType()) || collection.contentMimeTypes().contains(Collection::virtualMimeType());
        const bool resourceAllowsRequiredMimeTypes = AgentManager::self()->instance(collection.resource()).type().mimeTypes().toSet().contains(mimeTypes);

        const bool isReadOnlyForItems = (isItemAction && (!canCreateNewItems || !canContainRequiredMimeTypes));
        const bool isReadOnlyForCollections = (isCollectionAction && (!canCreateNewCollections || !canContainCollections || !resourceAllowsRequiredMimeTypes));

        return !(CollectionUtils::isStructural(collection) || isReadOnlyForItems || isReadOnlyForCollections);
    }

    void fillFoldersMenu(const Akonadi::Collection::List &selectedCollectionsList, const QSet<QString> &mimeTypes,  StandardActionManager::Type type, QMenu *menu,
                         const QAbstractItemModel *model, QModelIndex parentIndex)
    {
        const int rowCount = model->rowCount(parentIndex);

        for (int row = 0; row < rowCount; ++row) {
            const QModelIndex index = model->index(row, 0, parentIndex);
            const Collection collection = model->data(index, CollectionModel::CollectionRole).value<Collection>();

            if (collection.isVirtual()) {
                continue;
            }

            const bool readOnly = !isWritableTargetCollectionForMimeTypes(collection, mimeTypes, type);
            const bool collectionIsSelected = selectedCollectionsList.contains(collection);

            QString label = model->data(index).toString();
            label.replace(QLatin1String("&"), QLatin1String("&&"));

            const QIcon icon = model->data(index, Qt::DecorationRole).value<QIcon>();

            if (model->rowCount(index) > 0) {
                // new level
                QMenu *popup = new QMenu(menu);
                const bool moveAction = (type == MoveCollectionToMenu || type == MoveItemToMenu);
                popup->setObjectName(QString::fromUtf8("subMenu"));
                popup->setTitle(label);
                popup->setIcon(icon);

                fillFoldersMenu(selectedCollectionsList, mimeTypes, type, popup, model, index);

                if (!readOnly) {
                    popup->addSeparator();

                    QAction *action = popup->addAction(moveAction ? i18n("Move to This Folder") : i18n("Copy to This Folder"));
                    action->setData(QVariant::fromValue<QModelIndex>(index));
                }

                menu->addMenu(popup);

            } else {
                // insert an item
                QAction *action = menu->addAction(icon, label);
                action->setData(QVariant::fromValue<QModelIndex>(index));
                action->setEnabled(!readOnly && !collectionIsSelected);
            }
        }
    }

    void checkModelsConsistency()
    {
        if (favoritesModel == 0 || favoriteSelectionModel == 0) {
            // No need to check when the favorite collections feature is not used
            return;
        }

        // find the base ETM of the favourites view
        const QAbstractItemModel *favModel = favoritesModel;
        while (const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel *>(favModel)) {
            favModel = proxy->sourceModel();
        }

        // Check that the collection selection model maps to the same
        // EntityTreeModel than favoritesModel
        if (collectionSelectionModel != 0) {
            const QAbstractItemModel *model = collectionSelectionModel->model();
            while (const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel *>(model)) {
                model = proxy->sourceModel();
            }

            Q_ASSERT(model == favModel);
        }

        // Check that the favorite selection model maps to favoritesModel
        const QAbstractItemModel *model = favoriteSelectionModel->model();
        while (const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel *>(model)) {
            model = proxy->sourceModel();
        }
        Q_ASSERT(model == favModel);
    }

    void markCutAction(QMimeData *mimeData, bool cut) const
    {
        if (!cut) {
            return;
        }

        const QByteArray cutSelectionData = "1"; //krazy:exclude=doublequote_chars
        mimeData->setData(QLatin1String("application/x-kde.akonadi-cutselection"), cutSelectionData);
    }

    bool isCutAction(const QMimeData *mimeData) const
    {
        const QByteArray data = mimeData->data(QLatin1String("application/x-kde.akonadi-cutselection"));
        if (data.isEmpty()) {
            return false;
        } else {
            return (data.at(0) == '1');   // true if 1
        }
    }

    void setContextText(StandardActionManager::Type type, StandardActionManager::TextContext context, const QString &data)
    {
        ContextTextEntry entry;
        entry.text = data;

        contextTexts[type].insert(context, entry);
    }

    void setContextText(StandardActionManager::Type type, StandardActionManager::TextContext context, const KLocalizedString &data)
    {
        ContextTextEntry entry;
        entry.localizedText = data;

        contextTexts[type].insert(context, entry);
    }

    QString contextText(StandardActionManager::Type type, StandardActionManager::TextContext context) const
    {
        return contextTexts[type].value(context).text;
    }

    QString contextText(StandardActionManager::Type type, StandardActionManager::TextContext context, const QString &value) const
    {
        KLocalizedString text = contextTexts[type].value(context).localizedText;
        if (text.isEmpty()) {
            return contextTexts[type].value(context).text;
        }

        return text.subs(value).toString();
    }

    QString contextText(StandardActionManager::Type type, StandardActionManager::TextContext context, int count, const QString &value) const
    {
        KLocalizedString text = contextTexts[type].value(context).localizedText;
        if (text.isEmpty()) {
            return contextTexts[type].value(context).text;
        }

        const QString str = text.subs(count).toString();
        const int argCount = str.count(QRegExp(QLatin1String("%[0-9]")));
        if (argCount > 0) {
            return text.subs(count).subs(value).toString();
        } else {
            return text.subs(count).toString();
        }
    }

    StandardActionManager *q;
    KActionCollection *actionCollection;
    QWidget *parentWidget;
    QItemSelectionModel *collectionSelectionModel;
    QItemSelectionModel *itemSelectionModel;
    FavoriteCollectionsModel *favoritesModel;
    QItemSelectionModel *favoriteSelectionModel;
    bool insideSelectionSlot;
    QVector<KAction *> actions;
    QHash<StandardActionManager::Type, KLocalizedString> pluralLabels;
    QHash<StandardActionManager::Type, KLocalizedString> pluralIconLabels;

    struct ContextTextEntry
    {
        QString text;
        KLocalizedString localizedText;
        bool isLocalized;
    };

    typedef QHash<StandardActionManager::TextContext, ContextTextEntry> ContextTexts;
    QHash<StandardActionManager::Type, ContextTexts> contextTexts;

    ActionStateManager mActionStateManager;

    QStringList mMimeTypeFilter;
    QStringList mCapabilityFilter;
    QStringList mCollectionPropertiesPageNames;
    QMap<StandardActionManager::Type, QWeakPointer<RecentCollectionAction> > mRecentCollectionsMenu;
};

//@endcond

StandardActionManager::StandardActionManager(KActionCollection *actionCollection, QWidget *parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->parentWidget = parent;
    d->actionCollection = actionCollection;
    d->mActionStateManager.setReceiver(this);
#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)), SLOT(clipboardChanged(QClipboard::Mode)));
#endif
}

StandardActionManager::~ StandardActionManager()
{
    delete d;
}

void StandardActionManager::setCollectionSelectionModel(QItemSelectionModel *selectionModel)
{
    d->collectionSelectionModel = selectionModel;
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(collectionSelectionChanged()));

    d->checkModelsConsistency();
}

void StandardActionManager::setItemSelectionModel(QItemSelectionModel *selectionModel)
{
    d->itemSelectionModel = selectionModel;
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(updateActions()));
}

void StandardActionManager::setFavoriteCollectionsModel(FavoriteCollectionsModel *favoritesModel)
{
    d->favoritesModel = favoritesModel;
    d->checkModelsConsistency();
}

void StandardActionManager::setFavoriteSelectionModel(QItemSelectionModel *selectionModel)
{
    d->favoriteSelectionModel = selectionModel;
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(favoriteSelectionChanged()));
    d->checkModelsConsistency();
}

KAction *StandardActionManager::createAction(Type type)
{
    Q_ASSERT(type < LastType);
    if (d->actions[type]) {
        return d->actions[type];
    }
    KAction *action = 0;
    switch (standardActionData[type].actionType) {
    case NormalAction:
    case ActionWithAlternative:
        action = new KAction(d->parentWidget);
        break;
    case ActionAlternative:
        d->actions[type] = d->actions[type - 1];
        Q_ASSERT(d->actions[type]);
        if ((LastType > type + 1) && (standardActionData[type + 1].actionType == ActionAlternative)) {
            createAction(static_cast<Type>(type + 1)); //ensure that alternative actions are initialized when not created by createAllActions
        }
        return d->actions[type];
    case MenuAction:
        action = new KActionMenu(d->parentWidget);
        break;
    case ToggleAction:
        action = new KToggleAction(d->parentWidget);
        break;
    }

    if (d->pluralLabels.contains(type) && !d->pluralLabels.value(type).isEmpty()) {
        action->setText(d->pluralLabels.value(type).subs(1).toString());
    } else if (standardActionData[type].label) {
        action->setText(i18n(standardActionData[type].label));
    }

    if (d->pluralIconLabels.contains(type) && !d->pluralIconLabels.value(type).isEmpty()) {
        action->setIconText(d->pluralIconLabels.value(type).subs(1).toString());
    } else if (standardActionData[type].iconLabel) {
        action->setIconText(i18n(standardActionData[type].iconLabel));
    }

    if (standardActionData[type].icon) {
        action->setIcon(KIcon(QString::fromLatin1(standardActionData[type].icon)));
    }

    action->setShortcut(standardActionData[type].shortcut);

    if (standardActionData[type].slot) {
        switch (standardActionData[type].actionType) {
        case NormalAction:
        case ActionWithAlternative:
            connect(action, SIGNAL(triggered()), standardActionData[type].slot);
            break;
        case MenuAction: {
            KActionMenu *actionMenu = qobject_cast<KActionMenu *>(action);
            connect(actionMenu->menu(), SIGNAL(triggered(QAction*)), standardActionData[type].slot);
            break;
        }
        case ToggleAction: {
            connect(action, SIGNAL(triggered(bool)), standardActionData[type].slot);
            break;
        }
        case ActionAlternative:
            Q_ASSERT(0);
        }
    }

    if (type == ToggleWorkOffline) {
        // inititalize the action state with information from config file
        disconnect(action, SIGNAL(triggered(bool)), this, standardActionData[type].slot);
        action->setChecked(workOffline());
        connect(action, SIGNAL(triggered(bool)), this, standardActionData[type].slot);

        //TODO: find a way to check for updates to the config file
    }

    Q_ASSERT(standardActionData[type].name);
    d->actionCollection->addAction(QString::fromLatin1(standardActionData[type].name), action);
    d->actions[type] = action;
    if ((standardActionData[type].actionType == ActionWithAlternative) && (standardActionData[type + 1].actionType == ActionAlternative)) {
        createAction(static_cast<Type>(type + 1)); //ensure that alternative actions are initialized when not created by createAllActions
    }
    d->updateActions();
    return action;
}

void StandardActionManager::createAllActions()
{
    for (uint i = 0; i < LastType; ++i) {
        createAction((Type)i);
    }
}

KAction *StandardActionManager::action(Type type) const
{
    Q_ASSERT(type < LastType);
    return d->actions[type];
}

void StandardActionManager::setActionText(Type type, const KLocalizedString &text)
{
    Q_ASSERT(type < LastType);
    d->pluralLabels.insert(type, text);
    d->updateActions();
}

void StandardActionManager::interceptAction(Type type, bool intercept)
{
    Q_ASSERT(type < LastType);

    const KAction *action = d->actions[type];

    if (!action) {
        return;
    }

    if (intercept) {
        disconnect(action, SIGNAL(triggered()), this, standardActionData[type].slot);
    } else {
        connect(action, SIGNAL(triggered()), standardActionData[type].slot);
    }
}

Akonadi::Collection::List StandardActionManager::selectedCollections() const
{
    Collection::List collections;

    if (!d->collectionSelectionModel) {
        return collections;
    }

    foreach (const QModelIndex &index, safeSelectedRows(d->collectionSelectionModel)) {
        const Collection collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        if (collection.isValid()) {
            collections << collection;
        }
    }

    return collections;
}

Item::List StandardActionManager::selectedItems() const
{
    Item::List items;

    if (!d->itemSelectionModel) {
        return items;
    }

    foreach (const QModelIndex &index, safeSelectedRows(d->itemSelectionModel)) {
        const Item item = index.data(EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            items << item;
        }
    }

    return items;
}

void StandardActionManager::setContextText(Type type, TextContext context, const QString &text)
{
    d->setContextText(type, context, text);
}

void StandardActionManager::setContextText(Type type, TextContext context, const KLocalizedString &text)
{
    d->setContextText(type, context, text);
}

void StandardActionManager::setMimeTypeFilter(const QStringList &mimeTypes)
{
    d->mMimeTypeFilter = mimeTypes;
}

void StandardActionManager::setCapabilityFilter(const QStringList &capabilities)
{
    d->mCapabilityFilter = capabilities;
}

void StandardActionManager::setCollectionPropertiesPageNames(const QStringList &names)
{
    d->mCollectionPropertiesPageNames = names;
}

void StandardActionManager::createActionFolderMenu(QMenu *menu, Type type)
{
    d->createActionFolderMenu(menu, type);
}

#include "moc_standardactionmanager.cpp"
