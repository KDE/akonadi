/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
#include "collectionpropertiesdialog.h"
#include "collectionpropertiespage.h"
#include "collectionutils.h"
#include "entitydeletedattribute.h"
#include "entitytreemodel.h"
#include "favoritecollectionsmodel.h"
#include "itemdeletejob.h"
#include "pastehelper_p.h"
#include "recentcollectionaction_p.h"
#include "renamefavoritedialog_p.h"
#include "specialcollectionattribute.h"
#include "subscriptiondialog.h"
#include "trashjob.h"
#include "trashrestorejob.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KToggleAction>
#include <QAction>
#include <QIcon>
#include <QMenu>

#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QPointer>
#include <QRegularExpression>
#include <QTimer>

#include <KLazyLocalizedString>

using namespace Akonadi;

/// @cond PRIVATE

enum ActionType {
    NormalAction,
    ActionWithAlternative, // Normal action, but with an alternative state
    ActionAlternative, // Alternative state of the ActionWithAlternative
    MenuAction,
    ToggleAction
};

struct StandardActionData { // NOLINT(clang-analyzer-optin.performance.Padding) FIXME
    const char *name;
    const KLazyLocalizedString label;
    const KLazyLocalizedString iconLabel;
    const char *icon;
    const char *altIcon;
    int shortcut;
    const char *slot;
    ActionType actionType;
};

static const StandardActionData standardActionData[] = {
    {"akonadi_collection_create", kli18n("&New Folder..."), kli18n("New"), "folder-new", nullptr, 0, SLOT(slotCreateCollection()), NormalAction},
    {"akonadi_collection_copy", KLazyLocalizedString(), KLazyLocalizedString(), "edit-copy", nullptr, 0, SLOT(slotCopyCollections()), NormalAction},
    {"akonadi_collection_delete", kli18n("&Delete Folder"), kli18n("Delete"), "edit-delete", nullptr, 0, SLOT(slotDeleteCollection()), NormalAction},
    {"akonadi_collection_sync",
     kli18n("&Synchronize Folder"),
     kli18n("Synchronize"),
     "view-refresh",
     nullptr,
     Qt::Key_F5,
     SLOT(slotSynchronizeCollection()),
     NormalAction},
    {"akonadi_collection_properties",
     kli18n("Folder &Properties"),
     kli18n("Properties"),
     "configure",
     nullptr,
     0,
     SLOT(slotCollectionProperties()),
     NormalAction},
    {"akonadi_item_copy", KLazyLocalizedString(), KLazyLocalizedString(), "edit-copy", nullptr, 0, SLOT(slotCopyItems()), NormalAction},
    {"akonadi_paste", kli18n("&Paste"), kli18n("Paste"), "edit-paste", nullptr, Qt::CTRL | Qt::Key_V, SLOT(slotPaste()), NormalAction},
    {"akonadi_item_delete", KLazyLocalizedString(), KLazyLocalizedString(), "edit-delete", nullptr, 0, SLOT(slotDeleteItems()), NormalAction},
    {"akonadi_manage_local_subscriptions",
     kli18n("Manage Local &Subscriptions..."),
     kli18n("Manage Local Subscriptions"),
     "folder-bookmarks",
     nullptr,
     0,
     SLOT(slotLocalSubscription()),
     NormalAction},
    {"akonadi_collection_add_to_favorites",
     kli18n("Add to Favorite Folders"),
     kli18n("Add to Favorite"),
     "bookmark-new",
     nullptr,
     0,
     SLOT(slotAddToFavorites()),
     NormalAction},
    {"akonadi_collection_remove_from_favorites",
     kli18n("Remove from Favorite Folders"),
     kli18n("Remove from Favorite"),
     "edit-delete",
     nullptr,
     0,
     SLOT(slotRemoveFromFavorites()),
     NormalAction},
    {"akonadi_collection_rename_favorite", kli18n("Rename Favorite..."), kli18n("Rename"), "edit-rename", nullptr, 0, SLOT(slotRenameFavorite()), NormalAction},
    {"akonadi_collection_copy_to_menu",
     kli18n("Copy Folder To..."),
     kli18n("Copy To"),
     "edit-copy",
     nullptr,
     0,
     SLOT(slotCopyCollectionTo(QAction *)),
     MenuAction},
    {"akonadi_item_copy_to_menu", kli18n("Copy Item To..."), kli18n("Copy To"), "edit-copy", nullptr, 0, SLOT(slotCopyItemTo(QAction *)), MenuAction},
    {"akonadi_item_move_to_menu", kli18n("Move Item To..."), kli18n("Move To"), "edit-move", "go-jump", 0, SLOT(slotMoveItemTo(QAction *)), MenuAction},
    {"akonadi_collection_move_to_menu",
     kli18n("Move Folder To..."),
     kli18n("Move To"),
     "edit-move",
     "go-jump",
     0,
     SLOT(slotMoveCollectionTo(QAction *)),
     MenuAction},
    {"akonadi_item_cut", kli18n("&Cut Item"), kli18n("Cut"), "edit-cut", nullptr, Qt::CTRL | Qt::Key_X, SLOT(slotCutItems()), NormalAction},
    {"akonadi_collection_cut", kli18n("&Cut Folder"), kli18n("Cut"), "edit-cut", nullptr, Qt::CTRL | Qt::Key_X, SLOT(slotCutCollections()), NormalAction},
    {"akonadi_resource_create", kli18n("Create Resource"), KLazyLocalizedString(), "folder-new", nullptr, 0, SLOT(slotCreateResource()), NormalAction},
    {"akonadi_resource_delete", kli18n("Delete Resource"), KLazyLocalizedString(), "edit-delete", nullptr, 0, SLOT(slotDeleteResource()), NormalAction},
    {"akonadi_resource_properties",
     kli18n("&Resource Properties"),
     kli18n("Properties"),
     "configure",
     nullptr,
     0,
     SLOT(slotResourceProperties()),
     NormalAction},
    {"akonadi_resource_synchronize",
     kli18n("Synchronize Resource"),
     kli18n("Synchronize"),
     "view-refresh",
     nullptr,
     0,
     SLOT(slotSynchronizeResource()),
     NormalAction},
    {"akonadi_work_offline", kli18n("Work Offline"), KLazyLocalizedString(), "user-offline", nullptr, 0, SLOT(slotToggleWorkOffline(bool)), ToggleAction},
    {"akonadi_collection_copy_to_dialog", kli18n("Copy Folder To..."), kli18n("Copy To"), "edit-copy", nullptr, 0, SLOT(slotCopyCollectionTo()), NormalAction},
    {"akonadi_collection_move_to_dialog",
     kli18n("Move Folder To..."),
     kli18n("Move To"),
     "edit-move",
     "go-jump",
     0,
     SLOT(slotMoveCollectionTo()),
     NormalAction},
    {"akonadi_item_copy_to_dialog", kli18n("Copy Item To..."), kli18n("Copy To"), "edit-copy", nullptr, 0, SLOT(slotCopyItemTo()), NormalAction},
    {"akonadi_item_move_to_dialog", kli18n("Move Item To..."), kli18n("Move To"), "edit-move", "go-jump", 0, SLOT(slotMoveItemTo()), NormalAction},
    {"akonadi_collection_sync_recursive",
     kli18n("&Synchronize Folder Recursively"),
     kli18n("Synchronize Recursively"),
     "view-refresh",
     nullptr,
     Qt::CTRL | Qt::Key_F5,
     SLOT(slotSynchronizeCollectionRecursive()),
     NormalAction},
    {"akonadi_move_collection_to_trash",
     kli18n("&Move Folder To Trash"),
     kli18n("Move Folder To Trash"),
     "edit-delete",
     nullptr,
     0,
     SLOT(slotMoveCollectionToTrash()),
     NormalAction},
    {"akonadi_move_item_to_trash",
     kli18n("&Move Item To Trash"),
     kli18n("Move Item To Trash"),
     "edit-delete",
     nullptr,
     0,
     SLOT(slotMoveItemToTrash()),
     NormalAction},
    {"akonadi_restore_collection_from_trash",
     kli18n("&Restore Folder From Trash"),
     kli18n("Restore Folder From Trash"),
     "view-refresh",
     nullptr,
     0,
     SLOT(slotRestoreCollectionFromTrash()),
     NormalAction},
    {"akonadi_restore_item_from_trash",
     kli18n("&Restore Item From Trash"),
     kli18n("Restore Item From Trash"),
     "view-refresh",
     nullptr,
     0,
     SLOT(slotRestoreItemFromTrash()),
     NormalAction},
    {"akonadi_collection_trash_restore",
     kli18n("&Restore Folder From Trash"),
     kli18n("Restore Folder From Trash"),
     "edit-delete",
     nullptr,
     0,
     SLOT(slotTrashRestoreCollection()),
     ActionWithAlternative},
    {nullptr, kli18n("&Restore Collection From Trash"), kli18n("Restore Collection From Trash"), "view-refresh", nullptr, 0, nullptr, ActionAlternative},
    {"akonadi_item_trash_restore",
     kli18n("&Restore Item From Trash"),
     kli18n("Restore Item From Trash"),
     "edit-delete",
     nullptr,
     0,
     SLOT(slotTrashRestoreItem()),
     ActionWithAlternative},
    {nullptr, kli18n("&Restore Item From Trash"), kli18n("Restore Item From Trash"), "view-refresh", nullptr, 0, nullptr, ActionAlternative},
    {"akonadi_collection_sync_favorite_folders",
     kli18n("&Synchronize Favorite Folders"),
     kli18n("Synchronize Favorite Folders"),
     "view-refresh",
     nullptr,
     Qt::CTRL | Qt::SHIFT | Qt::Key_L,
     SLOT(slotSynchronizeFavoriteCollections()),
     NormalAction},
    {"akonadi_resource_synchronize_collectiontree",
     kli18n("Synchronize Folder Tree"),
     kli18n("Synchronize"),
     "view-refresh",
     nullptr,
     0,
     SLOT(slotSynchronizeCollectionTree()),
     NormalAction},

};
static const int numStandardActionData = sizeof standardActionData / sizeof *standardActionData;

static QIcon standardActionDataIcon(const StandardActionData &data)
{
    if (data.altIcon) {
        return QIcon::fromTheme(QString::fromLatin1(data.icon), QIcon::fromTheme(QString::fromLatin1(data.altIcon)));
    }
    return QIcon::fromTheme(QString::fromLatin1(data.icon));
}

static_assert(numStandardActionData == StandardActionManager::LastType, "StandardActionData table does not match StandardActionManager types");

static bool canCreateCollection(const Akonadi::Collection &collection)
{
    return !!(collection.rights() & Akonadi::Collection::CanCreateCollection);
}

static void setWorkOffline(bool offline)
{
    KConfig config(QStringLiteral("akonadikderc"));
    KConfigGroup group(&config, QStringLiteral("Actions"));

    group.writeEntry("WorkOffline", offline);
}

static bool workOffline()
{
    KConfig config(QStringLiteral("akonadikderc"));
    const KConfigGroup group(&config, QStringLiteral("Actions"));

    return group.readEntry("WorkOffline", false);
}

static QModelIndexList safeSelectedRows(QItemSelectionModel *selectionModel)
{
    QModelIndexList selectedRows = selectionModel->selectedRows();
    if (!selectedRows.isEmpty()) {
        return selectedRows;
    }

    // try harder for selected rows that don't span the full row for some reason (e.g. due to buggy column adding proxy models etc)
    const auto selection = selectionModel->selection();
    for (const auto &range : selection) {
        if (!range.isValid() || range.isEmpty()) {
            continue;
        }
        const QModelIndex parent = range.parent();
        for (int row = range.top(); row <= range.bottom(); ++row) {
            const QModelIndex index = range.model()->index(row, range.left(), parent);
            const Qt::ItemFlags flags = range.model()->flags(index);
            if ((flags & Qt::ItemIsSelectable) && (flags & Qt::ItemIsEnabled)) {
                selectedRows.push_back(index);
            }
        }
    }

    return selectedRows;
}

/**
 * @internal
 */
class Akonadi::StandardActionManagerPrivate
{
public:
    explicit StandardActionManagerPrivate(StandardActionManager *parent)
        : q(parent)
        , actionCollection(nullptr)
        , parentWidget(nullptr)
        , collectionSelectionModel(nullptr)
        , itemSelectionModel(nullptr)
        , favoritesModel(nullptr)
        , favoriteSelectionModel(nullptr)
        , insideSelectionSlot(false)
    {
        actions.fill(nullptr, StandardActionManager::LastType);

        pluralLabels.insert(StandardActionManager::CopyCollections, ki18np("&Copy Folder", "&Copy %1 Folders"));
        pluralLabels.insert(StandardActionManager::CopyItems, ki18np("&Copy Item", "&Copy %1 Items"));
        pluralLabels.insert(StandardActionManager::CutItems, ki18ncp("@action", "&Cut Item", "&Cut %1 Items"));
        pluralLabels.insert(StandardActionManager::CutCollections, ki18ncp("@action", "&Cut Folder", "&Cut %1 Folders"));
        pluralLabels.insert(StandardActionManager::DeleteItems, ki18np("&Delete Item", "&Delete %1 Items"));
        pluralLabels.insert(StandardActionManager::DeleteCollections, ki18ncp("@action", "&Delete Folder", "&Delete %1 Folders"));
        pluralLabels.insert(StandardActionManager::SynchronizeCollections, ki18ncp("@action", "&Synchronize Folder", "&Synchronize %1 Folders"));
        pluralLabels.insert(StandardActionManager::DeleteResources, ki18np("&Delete Resource", "&Delete %1 Resources"));
        pluralLabels.insert(StandardActionManager::SynchronizeResources, ki18np("&Synchronize Resource", "&Synchronize %1 Resources"));

        pluralIconLabels.insert(StandardActionManager::CopyCollections, ki18np("Copy Folder", "Copy %1 Folders"));
        pluralIconLabels.insert(StandardActionManager::CopyItems, ki18np("Copy Item", "Copy %1 Items"));
        pluralIconLabels.insert(StandardActionManager::CutItems, ki18np("Cut Item", "Cut %1 Items"));
        pluralIconLabels.insert(StandardActionManager::CutCollections, ki18np("Cut Folder", "Cut %1 Folders"));
        pluralIconLabels.insert(StandardActionManager::DeleteItems, ki18np("Delete Item", "Delete %1 Items"));
        pluralIconLabels.insert(StandardActionManager::DeleteCollections, ki18np("Delete Folder", "Delete %1 Folders"));
        pluralIconLabels.insert(StandardActionManager::SynchronizeCollections, ki18np("Synchronize Folder", "Synchronize %1 Folders"));
        pluralIconLabels.insert(StandardActionManager::DeleteResources, ki18ncp("@action", "Delete Resource", "Delete %1 Resources"));
        pluralIconLabels.insert(StandardActionManager::SynchronizeResources, ki18ncp("@action", "Synchronize Resource", "Synchronize %1 Resources"));

        setContextText(StandardActionManager::CreateCollection, StandardActionManager::DialogTitle, i18nc("@title:window", "New Folder"));
        setContextText(StandardActionManager::CreateCollection, StandardActionManager::DialogText, i18nc("@label:textbox name of Akonadi folder", "Name"));
        setContextText(StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageText, ki18n("Could not create folder: %1"));
        setContextText(StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageTitle, i18nc("@title:window", "Folder Creation Failed"));

        setContextText(
            StandardActionManager::DeleteCollections,
            StandardActionManager::MessageBoxText,
            ki18np("Do you really want to delete this folder and all its sub-folders?", "Do you really want to delete %1 folders and all their sub-folders?"));
        setContextText(StandardActionManager::DeleteCollections,
                       StandardActionManager::MessageBoxTitle,
                       ki18ncp("@title:window", "Delete Folder?", "Delete Folders?"));
        setContextText(StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageText, ki18n("Could not delete folder: %1"));
        setContextText(StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageTitle, i18nc("@title:window", "Folder Deletion Failed"));

        setContextText(StandardActionManager::CollectionProperties, StandardActionManager::DialogTitle, ki18nc("@title:window", "Properties of Folder %1"));

        setContextText(StandardActionManager::DeleteItems,
                       StandardActionManager::MessageBoxText,
                       ki18np("Do you really want to delete the selected item?", "Do you really want to delete %1 items?"));
        setContextText(StandardActionManager::DeleteItems, StandardActionManager::MessageBoxTitle, ki18ncp("@title:window", "Delete Item?", "Delete Items?"));
        setContextText(StandardActionManager::DeleteItems, StandardActionManager::ErrorMessageText, ki18n("Could not delete item: %1"));
        setContextText(StandardActionManager::DeleteItems, StandardActionManager::ErrorMessageTitle, i18nc("@title:window", "Item Deletion Failed"));

        setContextText(StandardActionManager::RenameFavoriteCollection, StandardActionManager::DialogTitle, i18nc("@title:window", "Rename Favorite"));
        setContextText(StandardActionManager::RenameFavoriteCollection, StandardActionManager::DialogText, i18nc("@label:textbox name of the folder", "Name:"));

        setContextText(StandardActionManager::CreateResource, StandardActionManager::DialogTitle, i18nc("@title:window", "New Resource"));
        setContextText(StandardActionManager::CreateResource, StandardActionManager::ErrorMessageText, ki18n("Could not create resource: %1"));
        setContextText(StandardActionManager::CreateResource, StandardActionManager::ErrorMessageTitle, i18nc("@title:window", "Resource Creation Failed"));

        setContextText(StandardActionManager::DeleteResources,
                       StandardActionManager::MessageBoxText,
                       ki18np("Do you really want to delete this resource?", "Do you really want to delete %1 resources?"));
        setContextText(StandardActionManager::DeleteResources,
                       StandardActionManager::MessageBoxTitle,
                       ki18ncp("@title:window", "Delete Resource?", "Delete Resources?"));

        setContextText(StandardActionManager::Paste, StandardActionManager::ErrorMessageText, ki18n("Could not paste data: %1"));
        setContextText(StandardActionManager::Paste, StandardActionManager::ErrorMessageTitle, i18nc("@title:window", "Paste Failed"));

        mDelayedUpdateTimer.setSingleShot(true);
        QObject::connect(&mDelayedUpdateTimer, &QTimer::timeout, q, [this]() {
            updateActions();
        });

        qRegisterMetaType<Akonadi::Item::List>("Akonadi::Item::List");
    }

    void enableAction(int type, bool enable) // private slot, called by ActionStateManager
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
        auto actionMenu = qobject_cast<KActionMenu *>(actions[type]);
        if (actionMenu) {
            // get rid of the submenus, they are re-created in enableAction. clear() is not enough, doesn't remove the submenu object instances.
            QMenu *menu = actionMenu->menu();
            // Not necessary to delete and recreate menu when it was not created
            if (menu->property("actionType").isValid() && menu->isEmpty()) {
                return;
            }
            mRecentCollectionsMenu.remove(type);
            delete menu;
            menu = new QMenu();

            menu->setProperty("actionType", static_cast<int>(type));
            q->connect(menu, &QMenu::aboutToShow, q, [this]() {
                aboutToShowMenu();
            });
            q->connect(menu, SIGNAL(triggered(QAction *)), standardActionData[type].slot); // clazy:exclude=old-style-connect
            actionMenu->setMenu(menu);
        }
    }

    void aboutToShowMenu()
    {
        auto menu = qobject_cast<QMenu *>(q->sender());
        if (!menu) {
            return;
        }

        if (!menu->isEmpty()) {
            return;
        }
        // collect all selected collections
        const Akonadi::Collection::List selectedCollectionsList = selectedCollections();
        const StandardActionManager::Type type = static_cast<StandardActionManager::Type>(menu->property("actionType").toInt());

        QPointer<RecentCollectionAction> recentCollection = new RecentCollectionAction(type, selectedCollectionsList, collectionSelectionModel->model(), menu);
        mRecentCollectionsMenu.insert(type, recentCollection);
        const QSet<QString> mimeTypes = mimeTypesOfSelection(type);
        fillFoldersMenu(selectedCollectionsList, mimeTypes, type, menu, collectionSelectionModel->model(), QModelIndex());
    }

    void createActionFolderMenu(QMenu *menu, StandardActionManager::Type type)
    {
        if (type == StandardActionManager::CopyCollectionToMenu || type == StandardActionManager::CopyItemToMenu
            || type == StandardActionManager::MoveItemToMenu || type == StandardActionManager::MoveCollectionToMenu) {
            new RecentCollectionAction(type, Akonadi::Collection::List(), collectionSelectionModel->model(), menu);
            Collection::List selectedCollectionsList = selectedCollections();
            const QSet<QString> mimeTypes = mimeTypesOfSelection(type);
            fillFoldersMenu(selectedCollectionsList, mimeTypes, type, menu, collectionSelectionModel->model(), QModelIndex());
        }
    }

    void updateAlternatingAction(int type) // private slot, called by ActionStateManager
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
            actions[type]->setText(standardActionData[type].label.toString());
            actions[type]->setIcon(standardActionDataIcon(standardActionData[type]));

            if (pluralLabels.contains(type) && !pluralLabels.value(type).isEmpty()) {
                actions[type]->setText(pluralLabels.value(type).subs(1).toString());
            } else if (!standardActionData[type].label.isEmpty()) {
                actions[type]->setText(standardActionData[type].label.toString());
            }
            if (pluralIconLabels.contains(type) && !pluralIconLabels.value(type).isEmpty()) {
                actions[type]->setIconText(pluralIconLabels.value(type).subs(1).toString());
            } else if (!standardActionData[type].iconLabel.isEmpty()) {
                actions[type]->setIconText(standardActionData[type].iconLabel.toString());
            }
            if (standardActionData[type].icon) {
                actions[type]->setIcon(standardActionDataIcon(standardActionData[type]));
            }

            // actions[type]->setShortcut( standardActionData[type].shortcut );

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

    void updatePluralLabel(int type, int count) // private slot, called by ActionStateManager
    {
        updatePluralLabel(static_cast<StandardActionManager::Type>(type), count);
    }

    void updatePluralLabel(StandardActionManager::Type type, int count) // private slot, called by ActionStateManager
    {
        Q_ASSERT(type < StandardActionManager::LastType);
        if (actions[type] && pluralLabels.contains(type) && !pluralLabels.value(type).isEmpty()) {
            actions[type]->setText(pluralLabels.value(type).subs(qMax(count, 1)).toString());
        }
    }

    bool isFavoriteCollection(const Akonadi::Collection &collection) const // private slot, called by ActionStateManager
    {
        if (!favoritesModel) {
            return false;
        }

        return favoritesModel->collectionIds().contains(collection.id());
    }

    void encodeToClipboard(QItemSelectionModel *selectionModel, bool cut = false)
    {
        Q_ASSERT(selectionModel);
        if (safeSelectedRows(selectionModel).isEmpty()) {
            return;
        }

#ifndef QT_NO_CLIPBOARD
        auto model = const_cast<QAbstractItemModel *>(selectionModel->model());
        QMimeData *mimeData = selectionModel->model()->mimeData(safeSelectedRows(selectionModel));
        model->setData(QModelIndex(), false, EntityTreeModel::PendingCutRole);
        markCutAction(mimeData, cut);
        QApplication::clipboard()->setMimeData(mimeData);
        if (cut) {
            const auto rows = safeSelectedRows(selectionModel);
            for (const auto &index : rows) {
                model->setData(index, true, EntityTreeModel::PendingCutRole);
            }
        }
#endif
    }

    static Akonadi::Collection::List collectionsForIndexes(const QModelIndexList &list)
    {
        Akonadi::Collection::List collectionList;
        for (const QModelIndex &index : list) {
            auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
            if (!collection.isValid()) {
                continue;
            }

            const auto parentCollection = index.data(EntityTreeModel::ParentCollectionRole).value<Collection>();
            collection.setParentCollection(parentCollection);

            collectionList << std::move(collection);
        }
        return collectionList;
    }

    void delayedUpdateActions()
    {
        // Compress changes (e.g. when deleting many rows, do this only once)
        mDelayedUpdateTimer.start(0);
    }

    void updateActions()
    {
        // favorite collections
        Collection::List selectedFavoriteCollectionsList;
        if (favoriteSelectionModel) {
            const QModelIndexList rows = safeSelectedRows(favoriteSelectionModel);
            selectedFavoriteCollectionsList = collectionsForIndexes(rows);
        }

        // collect all selected collections
        Collection::List selectedCollectionsList;
        if (collectionSelectionModel) {
            const QModelIndexList rows = safeSelectedRows(collectionSelectionModel);
            selectedCollectionsList = collectionsForIndexes(rows);
        }

        // collect all selected items
        Item::List selectedItems;
        if (itemSelectionModel) {
            const QModelIndexList rows = safeSelectedRows(itemSelectionModel);
            for (const QModelIndex &index : rows) {
                Item item = index.data(EntityTreeModel::ItemRole).value<Item>();
                if (!item.isValid()) {
                    continue;
                }

                const auto parentCollection = index.data(EntityTreeModel::ParentCollectionRole).value<Collection>();
                item.setParentCollection(parentCollection);

                selectedItems << item;
            }
        }

        mActionStateManager.updateState(selectedCollectionsList, selectedFavoriteCollectionsList, selectedItems);
        if (favoritesModel) {
            enableAction(StandardActionManager::SynchronizeFavoriteCollections, (favoritesModel->rowCount() > 0));
        }
        Q_EMIT q->selectionsChanged(selectedCollectionsList, selectedFavoriteCollectionsList, selectedItems);
        Q_EMIT q->actionStateUpdated();
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
        const auto proxy = qobject_cast<const QAbstractProxyModel *>(model);
        if (proxy) {
            return mapToEntityTreeModel(proxy->sourceModel(), proxy->mapSelectionToSource(selection));
        } else {
            return selection;
        }
    }

    QItemSelection mapFromEntityTreeModel(const QAbstractItemModel *model, const QItemSelection &selection) const
    {
        const auto proxy = qobject_cast<const QAbstractProxyModel *>(model);
        if (proxy) {
            const QItemSelection select = mapFromEntityTreeModel(proxy->sourceModel(), selection);
            return proxy->mapSelectionFromSource(select);
        } else {
            return selection;
        }
    }

    // RAII class for setting insideSelectionSlot to true on entering, and false on exiting, the two slots below.
    class InsideSelectionSlotBlocker
    {
    public:
        explicit InsideSelectionSlotBlocker(StandardActionManagerPrivate *p)
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
        Q_DISABLE_COPY(InsideSelectionSlotBlocker)
        StandardActionManagerPrivate *const _p;
    };

    void collectionSelectionChanged()
    {
        if (insideSelectionSlot) {
            return;
        }
        InsideSelectionSlotBlocker block(this);
        if (favoriteSelectionModel) {
            QItemSelection selection = collectionSelectionModel->selection();
            selection = mapToEntityTreeModel(collectionSelectionModel->model(), selection);
            selection = mapFromEntityTreeModel(favoriteSelectionModel->model(), selection);

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

        selection = mapToEntityTreeModel(favoriteSelectionModel->model(), selection);
        selection = mapFromEntityTreeModel(collectionSelectionModel->model(), selection);

        InsideSelectionSlotBlocker block(this);
        collectionSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        // Also set the current index. This will trigger KMMainWidget::slotFolderChanged in kmail, which we want.
        if (!selection.indexes().isEmpty()) {
            collectionSelectionModel->setCurrentIndex(selection.indexes().first(), QItemSelectionModel::NoUpdate);
        }

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
        const auto parentCollection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        Q_ASSERT(parentCollection.isValid());

        if (!canCreateCollection(parentCollection)) {
            return;
        }

        bool ok = false;
        QString name = QInputDialog::getText(parentWidget,
                                             contextText(StandardActionManager::CreateCollection, StandardActionManager::DialogTitle),
                                             contextText(StandardActionManager::CreateCollection, StandardActionManager::DialogText),
                                             {},
                                             {},
                                             &ok);
        name = name.trimmed();
        if (name.isEmpty() || !ok) {
            return;
        }

        if (name.contains(QLatin1Char('/'))) {
            KMessageBox::error(parentWidget, i18n("We can not add \"/\" in folder name."), i18nc("@title:window", "Create New Folder Error"));
            return;
        }
        if (name.startsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char('.'))) {
            KMessageBox::error(parentWidget, i18n("We can not add \".\" at begin or end of folder name."), i18nc("@title:window", "Create New Folder Error"));
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
            collection.setContentMimeTypes(collection.contentMimeTypes() << Collection::virtualMimeType());
        }

        auto job = new CollectionCreateJob(collection);
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            collectionCreationResult(job);
        });
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
        const QModelIndexList indexes = safeSelectedRows(collectionSelectionModel);
        collections.reserve(indexes.count());

        for (const QModelIndex &index : indexes) {
            Q_ASSERT(index.isValid());
            const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
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
        const QString text = contextText(StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxText, collections.count(), collectionName);

        if (KMessageBox::questionYesNo(
                parentWidget,
                text,
                contextText(StandardActionManager::DeleteCollections, StandardActionManager::MessageBoxTitle, collections.count(), collectionName),
                KStandardGuiItem::del(),
                KStandardGuiItem::cancel(),
                QString(),
                KMessageBox::Dangerous)
            != KMessageBox::Yes) {
            return;
        }

        for (const Collection &collection : collections) {
            auto job = new CollectionDeleteJob(collection, q);
            q->connect(job, &CollectionDeleteJob::result, q, [this](KJob *job) {
                collectionDeletionResult(job);
            });
        }
    }

    void slotMoveCollectionToTrash()
    {
        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        for (const Collection &collection : collections) {
            auto job = new TrashJob(collection, q);
            q->connect(job, &TrashJob::result, q, [this](KJob *job) {
                moveCollectionToTrashResult(job);
            });
        }
    }

    void slotRestoreCollectionFromTrash()
    {
        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        for (const Collection &collection : collections) {
            auto job = new TrashRestoreJob(collection, q);
            q->connect(job, &TrashRestoreJob::result, q, [this](KJob *job) {
                moveCollectionToTrashResult(job);
            });
        }
    }

    Item::List selectedItems() const
    {
        Item::List items;

        Q_ASSERT(itemSelectionModel);
        const QModelIndexList indexes = safeSelectedRows(itemSelectionModel);
        items.reserve(indexes.count());

        for (const QModelIndex &index : indexes) {
            Q_ASSERT(index.isValid());
            const Item item = index.data(EntityTreeModel::ItemRole).value<Item>();
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

        auto job = new TrashJob(items, q);
        q->connect(job, &TrashJob::result, q, [this](KJob *job) {
            moveItemToTrashResult(job);
        });
    }

    void slotRestoreItemFromTrash()
    {
        const Item::List items = selectedItems();
        if (items.isEmpty()) {
            return;
        }

        auto job = new TrashRestoreJob(items, q);
        q->connect(job, &TrashRestoreJob::result, q, [this](KJob *job) {
            moveItemToTrashResult(job);
        });
    }

    void slotTrashRestoreCollection()
    {
        const Collection::List collections = selectedCollections();
        if (collections.isEmpty()) {
            return;
        }

        bool collectionsAreInTrash = false;
        for (const Collection &collection : collections) {
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
        for (const Item &item : items) {
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

        for (const Collection &collection : collections) {
            if (!testAndSetOnlineResources(collection)) {
                break;
            }
            AgentManager::self()->synchronizeCollection(collection, false);
        }
    }

    bool testAndSetOnlineResources(const Akonadi::Collection &collection)
    {
        // Shortcut for the Search resource, which is a virtual resource and thus
        // is always online (but AgentManager does not know about it, so it returns
        // an invalid AgentInstance, which is "offline").
        //
        // FIXME: AgentManager should return a valid AgentInstance even
        // for virtual resources, which would be always online.
        if (collection.resource() == QLatin1String("akonadi_search_resource")) {
            return true;
        }

        Akonadi::AgentInstance instance = Akonadi::AgentManager::self()->instance(collection.resource());
        if (!instance.isOnline()) {
            if (KMessageBox::questionYesNo(
                    parentWidget,
                    i18n("Before syncing folder \"%1\" it is necessary to have the resource online. Do you want to make it online?", collection.displayName()),
                    i18n("Account \"%1\" is offline", instance.name()),
                    KGuiItem(i18nc("@action:button", "Go Online")),
                    KStandardGuiItem::cancel())
                != KMessageBox::Yes) {
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

        for (const Collection &collection : collections) {
            if (!testAndSetOnlineResources(collection)) {
                break;
            }
            AgentManager::self()->synchronizeCollection(collection, true);
        }
    }

    void slotCollectionProperties() const
    {
        const QModelIndexList list = safeSelectedRows(collectionSelectionModel);
        if (list.isEmpty()) {
            return;
        }

        const QModelIndex index = list.first();
        Q_ASSERT(index.isValid());

        const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        Q_ASSERT(collection.isValid());

        auto dlg = new CollectionPropertiesDialog(collection, mCollectionPropertiesPageNames, parentWidget);
        dlg->setWindowTitle(contextText(StandardActionManager::CollectionProperties, StandardActionManager::DialogTitle, collection.displayName()));
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
        auto model = const_cast<QAbstractItemModel *>(collectionSelectionModel->model());
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
        const QModelIndexList indexes = safeSelectedRows(itemSelectionModel);
        items.reserve(indexes.count());
        for (const QModelIndex &index : indexes) {
            bool ok;
            const qlonglong id = index.data(EntityTreeModel::ItemIdRole).toLongLong(&ok);
            Q_ASSERT(ok);
            items << Item(id);
        }

        if (items.isEmpty()) {
            return;
        }

        QMetaObject::invokeMethod(
            q,
            [this, items] {
                slotDeleteItemsDeferred(items);
            },
            Qt::QueuedConnection);
    }

    void slotDeleteItemsDeferred(const Akonadi::Item::List &items)
    {
        Q_ASSERT(itemSelectionModel);

        if (KMessageBox::questionYesNo(parentWidget,
                                       contextText(StandardActionManager::DeleteItems, StandardActionManager::MessageBoxText, items.count(), QString()),
                                       contextText(StandardActionManager::DeleteItems, StandardActionManager::MessageBoxTitle, items.count(), QString()),
                                       KStandardGuiItem::del(),
                                       KStandardGuiItem::cancel(),
                                       QString(),
                                       KMessageBox::Dangerous)
            != KMessageBox::Yes) {
            return;
        }

        auto job = new ItemDeleteJob(items, q);
        q->connect(job, &ItemDeleteJob::result, q, [this](KJob *job) {
            itemDeletionResult(job);
        });
    }

    void slotLocalSubscription() const
    {
        auto dlg = new SubscriptionDialog(mMimeTypeFilter, parentWidget);
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

        for (const QModelIndex &index : list) {
            Q_ASSERT(index.isValid());
            const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
            Q_ASSERT(collection.isValid());

            favoritesModel->addCollection(collection);
        }

        updateActions();
    }

    void slotRemoveFromFavorites()
    {
        Q_ASSERT(favoriteSelectionModel);
        Q_ASSERT(favoritesModel);
        const QModelIndexList list = safeSelectedRows(favoriteSelectionModel);
        if (list.isEmpty()) {
            return;
        }

        for (const QModelIndex &index : list) {
            Q_ASSERT(index.isValid());
            const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
            Q_ASSERT(collection.isValid());

            favoritesModel->removeCollection(collection);
        }

        updateActions();
    }

    void slotRenameFavorite()
    {
        Q_ASSERT(favoriteSelectionModel);
        Q_ASSERT(favoritesModel);
        const QModelIndexList list = safeSelectedRows(favoriteSelectionModel);
        if (list.isEmpty()) {
            return;
        }
        const QModelIndex index = list.first();
        Q_ASSERT(index.isValid());
        const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        Q_ASSERT(collection.isValid());

        QPointer<RenameFavoriteDialog> dlg(
            new RenameFavoriteDialog(favoritesModel->favoriteLabel(collection), favoritesModel->defaultFavoriteLabel(collection), parentWidget));
        if (dlg->exec() == QDialog::Accepted) {
            favoritesModel->setFavoriteLabel(collection, dlg->newName());
        }
        delete dlg;
    }

    void slotSynchronizeFavoriteCollections()
    {
        Q_ASSERT(favoritesModel);
        const auto collections = favoritesModel->collections();
        for (const auto &collection : collections) {
            // there might be virtual collections in favorites which cannot be checked
            // so let's be safe here, agentmanager asserts otherwise
            if (!collection.resource().isEmpty()) {
                AgentManager::self()->synchronizeCollection(collection, false);
            }
        }
    }

    void slotCopyCollectionTo()
    {
        pasteTo(collectionSelectionModel, collectionSelectionModel->model(), StandardActionManager::CopyCollectionToMenu, Qt::CopyAction);
    }

    void slotCopyItemTo()
    {
        pasteTo(itemSelectionModel, collectionSelectionModel->model(), StandardActionManager::CopyItemToMenu, Qt::CopyAction);
    }

    void slotMoveCollectionTo()
    {
        pasteTo(collectionSelectionModel, collectionSelectionModel->model(), StandardActionManager::MoveCollectionToMenu, Qt::MoveAction);
    }

    void slotMoveItemTo()
    {
        pasteTo(itemSelectionModel, collectionSelectionModel->model(), StandardActionManager::MoveItemToMenu, Qt::MoveAction);
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
        const QModelIndexList lstIndexes = collectionSelectionModel->selection().indexes();
        for (const QModelIndex &index : lstIndexes) {
            Q_ASSERT(index.isValid());
            const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
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
        dlg->setWindowTitle(contextText(StandardActionManager::CreateResource, StandardActionManager::DialogTitle));

        for (const QString &mimeType : std::as_const(mMimeTypeFilter)) {
            dlg->agentFilterProxyModel()->addMimeTypeFilter(mimeType);
        }

        for (const QString &capability : std::as_const(mCapabilityFilter)) {
            dlg->agentFilterProxyModel()->addCapabilityFilter(capability);
        }

        if (dlg->exec() == QDialog::Accepted) {
            const AgentType agentType = dlg->agentType();

            if (agentType.isValid()) {
                auto job = new AgentInstanceCreateJob(agentType, q);
                q->connect(job, &KJob::result, q, [this](KJob *job) {
                    resourceCreationResult(job);
                });
                job->configure(parentWidget);
                job->start();
            }
        }
        delete dlg;
    }

    void slotDeleteResource() const
    {
        const AgentInstance::List instances = selectedAgentInstances();
        if (instances.isEmpty()) {
            return;
        }

        if (KMessageBox::questionYesNo(
                parentWidget,
                contextText(StandardActionManager::DeleteResources, StandardActionManager::MessageBoxText, instances.count(), instances.first().name()),
                contextText(StandardActionManager::DeleteResources, StandardActionManager::MessageBoxTitle, instances.count(), instances.first().name()),
                KStandardGuiItem::del(),
                KStandardGuiItem::cancel(),
                QString(),
                KMessageBox::Dangerous)
            != KMessageBox::Yes) {
            return;
        }

        for (const AgentInstance &instance : instances) {
            AgentManager::self()->removeInstance(instance);
        }
    }

    void slotSynchronizeResource() const
    {
        AgentInstance::List instances = selectedAgentInstances();
        for (AgentInstance &instance : instances) {
            instance.synchronize();
        }
    }

    void slotSynchronizeCollectionTree() const
    {
        AgentInstance::List instances = selectedAgentInstances();
        for (AgentInstance &instance : instances) {
            instance.synchronizeCollectionTree();
        }
    }

    void slotResourceProperties() const
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
        for (AgentInstance &instance : instances) {
            instance.setIsOnline(!offline);
        }
    }

    void pasteTo(QItemSelectionModel *selectionModel, const QAbstractItemModel *model, StandardActionManager::Type type, Qt::DropAction dropAction)
    {
        const QSet<QString> mimeTypes = mimeTypesOfSelection(type);

        QPointer<CollectionDialog> dlg(new CollectionDialog(const_cast<QAbstractItemModel *>(model)));
        dlg->setMimeTypeFilter(mimeTypes.values());

        if (type == StandardActionManager::CopyItemToMenu || type == StandardActionManager::MoveItemToMenu) {
            dlg->setAccessRightsFilter(Collection::CanCreateItem);
        } else if (type == StandardActionManager::CopyCollectionToMenu || type == StandardActionManager::MoveCollectionToMenu) {
            dlg->setAccessRightsFilter(Collection::CanCreateCollection);
        }

        if (dlg->exec() == QDialog::Accepted && dlg != nullptr) {
            const QModelIndex index = EntityTreeModel::modelIndexForCollection(collectionSelectionModel->model(), dlg->selectedCollection());
            if (!index.isValid()) {
                delete dlg;
                return;
            }

            const QMimeData *mimeData = selectionModel->model()->mimeData(safeSelectedRows(selectionModel));

            auto model = const_cast<QAbstractItemModel *>(index.model());
            model->dropMimeData(mimeData, dropAction, -1, -1, index);
            delete mimeData;
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

        const QModelIndex index = action->data().toModelIndex();
        Q_ASSERT(index.isValid());

        auto model = const_cast<QAbstractItemModel *>(index.model());
        const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        addRecentCollection(collection.id());
        model->dropMimeData(mimeData, dropAction, -1, -1, index);
        delete mimeData;
    }

    void addRecentCollection(Akonadi::Collection::Id id) const
    {
        QMapIterator<StandardActionManager::Type, QPointer<RecentCollectionAction>> item(mRecentCollectionsMenu);
        while (item.hasNext()) {
            item.next();
            if (item.value().data()) {
                item.value().data()->addRecentCollection(item.key(), id);
            }
        }
    }

    void collectionCreationResult(KJob *job) const
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::CreateCollection, StandardActionManager::ErrorMessageTitle));
        }
    }

    void collectionDeletionResult(KJob *job) const
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::DeleteCollections, StandardActionManager::ErrorMessageTitle));
        }
    }

    void moveCollectionToTrashResult(KJob *job) const
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::MoveCollectionsToTrash, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::MoveCollectionsToTrash, StandardActionManager::ErrorMessageTitle));
        }
    }

    void moveItemToTrashResult(KJob *job) const
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::MoveItemsToTrash, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::MoveItemsToTrash, StandardActionManager::ErrorMessageTitle));
        }
    }

    void itemDeletionResult(KJob *job) const
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::DeleteItems, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::DeleteItems, StandardActionManager::ErrorMessageTitle));
        }
    }

    void resourceCreationResult(KJob *job) const
    {
        if (job->error()) {
            KMessageBox::error(parentWidget,
                               contextText(StandardActionManager::CreateResource, StandardActionManager::ErrorMessageText, job->errorString()),
                               contextText(StandardActionManager::CreateResource, StandardActionManager::ErrorMessageTitle));
        }
    }

    void pasteResult(KJob *job) const
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

        const bool isItemAction = (type == StandardActionManager::CopyItemToMenu || type == StandardActionManager::MoveItemToMenu);
        const bool isCollectionAction = (type == StandardActionManager::CopyCollectionToMenu || type == StandardActionManager::MoveCollectionToMenu);

        if (isItemAction) {
            list = safeSelectedRows(itemSelectionModel);
            mimeTypes.reserve(list.count());
            for (const QModelIndex &index : std::as_const(list)) {
                mimeTypes << index.data(EntityTreeModel::MimeTypeRole).toString();
            }
        }

        if (isCollectionAction) {
            list = safeSelectedRows(collectionSelectionModel);
            for (const QModelIndex &index : std::as_const(list)) {
                const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();

                // The mimetypes that the selected collection can possibly contain
                const auto mimeTypesResult = AgentManager::self()->instance(collection.resource()).type().mimeTypes();
                mimeTypes = QSet<QString>(mimeTypesResult.begin(), mimeTypesResult.end());
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

        const bool isItemAction = (type == StandardActionManager::CopyItemToMenu || type == StandardActionManager::MoveItemToMenu);
        const bool isCollectionAction = (type == StandardActionManager::CopyCollectionToMenu || type == StandardActionManager::MoveCollectionToMenu);

        const auto contentMimeTypesList{collection.contentMimeTypes()};
        const QSet<QString> contentMimeTypesSet = QSet<QString>(contentMimeTypesList.cbegin(), contentMimeTypesList.cend());

        const bool canContainRequiredMimeTypes = contentMimeTypesSet.intersects(mimeTypes);
        const bool canCreateNewItems = (collection.rights() & Collection::CanCreateItem);

        const bool canCreateNewCollections = (collection.rights() & Collection::CanCreateCollection);
        const bool canContainCollections =
            collection.contentMimeTypes().contains(Collection::mimeType()) || collection.contentMimeTypes().contains(Collection::virtualMimeType());

        const auto mimeTypesList{AgentManager::self()->instance(collection.resource()).type().mimeTypes()};
        const QSet<QString> mimeTypesListSet = QSet<QString>(mimeTypesList.cbegin(), mimeTypesList.cend());

        const bool resourceAllowsRequiredMimeTypes = mimeTypesListSet.contains(mimeTypes);
        const bool isReadOnlyForItems = (isItemAction && (!canCreateNewItems || !canContainRequiredMimeTypes));
        const bool isReadOnlyForCollections = (isCollectionAction && (!canCreateNewCollections || !canContainCollections || !resourceAllowsRequiredMimeTypes));

        return !(CollectionUtils::isStructural(collection) || isReadOnlyForItems || isReadOnlyForCollections);
    }

    void fillFoldersMenu(const Akonadi::Collection::List &selectedCollectionsList,
                         const QSet<QString> &mimeTypes,
                         StandardActionManager::Type type,
                         QMenu *menu,
                         const QAbstractItemModel *model,
                         const QModelIndex &parentIndex)
    {
        const int rowCount = model->rowCount(parentIndex);

        for (int row = 0; row < rowCount; ++row) {
            const QModelIndex index = model->index(row, 0, parentIndex);
            const auto collection = model->data(index, EntityTreeModel::CollectionRole).value<Collection>();

            if (collection.isVirtual()) {
                continue;
            }

            const bool readOnly = !isWritableTargetCollectionForMimeTypes(collection, mimeTypes, type);
            const bool collectionIsSelected = selectedCollectionsList.contains(collection);
            if (type == StandardActionManager::MoveCollectionToMenu && collectionIsSelected) {
                continue;
            }

            QString label = model->data(index).toString();
            label.replace(QLatin1Char('&'), QStringLiteral("&&"));

            const auto icon = model->data(index, Qt::DecorationRole).value<QIcon>();

            if (model->rowCount(index) > 0) {
                // new level
                auto popup = new QMenu(menu);
                const bool moveAction = (type == StandardActionManager::MoveCollectionToMenu || type == StandardActionManager::MoveItemToMenu);
                popup->setObjectName(QStringLiteral("subMenu"));
                popup->setTitle(label);
                popup->setIcon(icon);

                fillFoldersMenu(selectedCollectionsList, mimeTypes, type, popup, model, index);
                if (!(type == StandardActionManager::CopyCollectionToMenu && collectionIsSelected)) {
                    if (!readOnly) {
                        popup->addSeparator();

                        QAction *action = popup->addAction(moveAction ? i18n("Move to This Folder") : i18n("Copy to This Folder"));
                        action->setData(QVariant::fromValue<QModelIndex>(index));
                    }
                }

                if (!popup->isEmpty()) {
                    menu->addMenu(popup);
                }

            } else {
                // insert an item
                QAction *action = menu->addAction(icon, label);
                action->setData(QVariant::fromValue<QModelIndex>(index));
                action->setEnabled(!readOnly && !collectionIsSelected);
            }
        }
    }

    void checkModelsConsistency() const
    {
        if (favoritesModel == nullptr || favoriteSelectionModel == nullptr) {
            // No need to check when the favorite collections feature is not used
            return;
        }

        // find the base ETM of the favourites view
        const QAbstractItemModel *favModel = favoritesModel;
        while (const auto proxy = qobject_cast<const QAbstractProxyModel *>(favModel)) {
            favModel = proxy->sourceModel();
        }

        // Check that the collection selection model maps to the same
        // EntityTreeModel than favoritesModel
        if (collectionSelectionModel != nullptr) {
            const QAbstractItemModel *model = collectionSelectionModel->model();
            while (const auto proxy = qobject_cast<const QAbstractProxyModel *>(model)) {
                model = proxy->sourceModel();
            }

            Q_ASSERT(model == favModel);
        }

        // Check that the favorite selection model maps to favoritesModel
        const QAbstractItemModel *model = favoriteSelectionModel->model();
        while (const auto proxy = qobject_cast<const QAbstractProxyModel *>(model)) {
            model = proxy->sourceModel();
        }
        Q_ASSERT(model == favModel);
    }

    void markCutAction(QMimeData *mimeData, bool cut) const
    {
        if (!cut) {
            return;
        }

        const QByteArray cutSelectionData = "1"; // krazy:exclude=doublequote_chars
        mimeData->setData(QStringLiteral("application/x-kde.akonadi-cutselection"), cutSelectionData);
    }

    bool isCutAction(const QMimeData *mimeData) const
    {
        const QByteArray data = mimeData->data(QStringLiteral("application/x-kde.akonadi-cutselection"));
        if (data.isEmpty()) {
            return false;
        } else {
            return (data.at(0) == '1'); // true if 1
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
        const int argCount = str.count(QRegularExpression(QStringLiteral("%[0-9]")));
        if (argCount > 0) {
            return text.subs(count).subs(value).toString();
        } else {
            return text.subs(count).toString();
        }
    }

    StandardActionManager *const q;
    KActionCollection *actionCollection;
    QWidget *parentWidget;
    QItemSelectionModel *collectionSelectionModel;
    QItemSelectionModel *itemSelectionModel;
    FavoriteCollectionsModel *favoritesModel;
    QItemSelectionModel *favoriteSelectionModel;
    bool insideSelectionSlot;
    QVector<QAction *> actions;
    QHash<StandardActionManager::Type, KLocalizedString> pluralLabels;
    QHash<StandardActionManager::Type, KLocalizedString> pluralIconLabels;
    QTimer mDelayedUpdateTimer;

    struct ContextTextEntry {
        QString text;
        KLocalizedString localizedText;
        bool isLocalized;
    };

    using ContextTexts = QHash<StandardActionManager::TextContext, ContextTextEntry>;
    QHash<StandardActionManager::Type, ContextTexts> contextTexts;

    ActionStateManager mActionStateManager;

    QStringList mMimeTypeFilter;
    QStringList mCapabilityFilter;
    QStringList mCollectionPropertiesPageNames;
    QMap<StandardActionManager::Type, QPointer<RecentCollectionAction>> mRecentCollectionsMenu;
};

/// @endcond

StandardActionManager::StandardActionManager(KActionCollection *actionCollection, QWidget *parent)
    : QObject(parent)
    , d(new StandardActionManagerPrivate(this))
{
    d->parentWidget = parent;
    d->actionCollection = actionCollection;
    d->mActionStateManager.setReceiver(this);
#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), &QClipboard::changed, this, [this](auto mode) {
        d->clipboardChanged(mode);
    });
#endif
}

StandardActionManager::~StandardActionManager() = default;

void StandardActionManager::setCollectionSelectionModel(QItemSelectionModel *selectionModel)
{
    d->collectionSelectionModel = selectionModel;
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, [this]() {
        d->collectionSelectionChanged();
    });

    d->checkModelsConsistency();
}

void StandardActionManager::setItemSelectionModel(QItemSelectionModel *selectionModel)
{
    d->itemSelectionModel = selectionModel;
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, [this]() {
        d->delayedUpdateActions();
    });
}

void StandardActionManager::setFavoriteCollectionsModel(FavoriteCollectionsModel *favoritesModel)
{
    d->favoritesModel = favoritesModel;
    d->checkModelsConsistency();
}

void StandardActionManager::setFavoriteSelectionModel(QItemSelectionModel *selectionModel)
{
    d->favoriteSelectionModel = selectionModel;
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, [this]() {
        d->favoriteSelectionChanged();
    });
    d->checkModelsConsistency();
}

QAction *StandardActionManager::createAction(Type type)
{
    Q_ASSERT(type < LastType);
    if (d->actions[type]) {
        return d->actions[type];
    }
    QAction *action = nullptr;
    switch (standardActionData[type].actionType) {
    case NormalAction:
    case ActionWithAlternative:
        action = new QAction(d->parentWidget);
        break;
    case ActionAlternative:
        d->actions[type] = d->actions[type - 1];
        Q_ASSERT(d->actions[type]);
        if ((LastType > type + 1) && (standardActionData[type + 1].actionType == ActionAlternative)) {
            createAction(static_cast<Type>(type + 1)); // ensure that alternative actions are initialized when not created by createAllActions
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
    } else if (!standardActionData[type].label.isEmpty()) {
        action->setText(standardActionData[type].label.toString());
    }
    if (d->pluralIconLabels.contains(type) && !d->pluralIconLabels.value(type).isEmpty()) {
        action->setIconText(d->pluralIconLabels.value(type).subs(1).toString());
    } else if (!standardActionData[type].iconLabel.isEmpty()) {
        action->setIconText(standardActionData[type].iconLabel.toString());
    }

    if (standardActionData[type].icon) {
        action->setIcon(standardActionDataIcon(standardActionData[type]));
    }
    if (d->actionCollection) {
        d->actionCollection->setDefaultShortcut(action, QKeySequence(standardActionData[type].shortcut));
    } else {
        action->setShortcut(standardActionData[type].shortcut);
    }

    if (standardActionData[type].slot) {
        switch (standardActionData[type].actionType) {
        case NormalAction:
        case ActionWithAlternative:
            connect(action, SIGNAL(triggered()), standardActionData[type].slot); // clazy:exclude=old-style-connect
            break;
        case MenuAction: {
            auto actionMenu = qobject_cast<KActionMenu *>(action);
            connect(actionMenu->menu(), SIGNAL(triggered(QAction *)), standardActionData[type].slot); // clazy:exclude=old-style-connect
            break;
        }
        case ToggleAction: {
            connect(action, SIGNAL(triggered(bool)), standardActionData[type].slot); // clazy:exclude=old-style-connect
            break;
        }
        case ActionAlternative:
            Q_ASSERT(0);
        }
    }

    if (type == ToggleWorkOffline) {
        // inititalize the action state with information from config file
        disconnect(action, SIGNAL(triggered(bool)), this, standardActionData[type].slot); // clazy:exclude=old-style-connect
        action->setChecked(workOffline());
        connect(action, SIGNAL(triggered(bool)), this, standardActionData[type].slot); // clazy:exclude=old-style-connect

        // TODO: find a way to check for updates to the config file
    }

    Q_ASSERT(standardActionData[type].name);
    Q_ASSERT(d->actionCollection);
    d->actionCollection->addAction(QString::fromLatin1(standardActionData[type].name), action);
    d->actions[type] = action;
    if ((standardActionData[type].actionType == ActionWithAlternative) && (standardActionData[type + 1].actionType == ActionAlternative)) {
        createAction(static_cast<Type>(type + 1)); // ensure that alternative actions are initialized when not created by createAllActions
    }
    d->updateActions();
    return action;
}

void StandardActionManager::createAllActions()
{
    for (uint i = 0; i < LastType; ++i) {
        createAction(static_cast<Type>(i));
    }
}

QAction *StandardActionManager::action(Type type) const
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

    const QAction *action = d->actions[type];

    if (!action) {
        return;
    }

    if (intercept) {
        disconnect(action, SIGNAL(triggered()), this, standardActionData[type].slot); // clazy:exclude=old-style-connect
    } else {
        connect(action, SIGNAL(triggered()), standardActionData[type].slot); // clazy:exclude=old-style-connect
    }
}

Akonadi::Collection::List StandardActionManager::selectedCollections() const
{
    Collection::List collections;

    if (!d->collectionSelectionModel) {
        return collections;
    }

    const QModelIndexList lst = safeSelectedRows(d->collectionSelectionModel);
    for (const QModelIndex &index : lst) {
        const auto collection = index.data(EntityTreeModel::CollectionRole).value<Collection>();
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
    const QModelIndexList lst = safeSelectedRows(d->itemSelectionModel);
    for (const QModelIndex &index : lst) {
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

void StandardActionManager::addRecentCollection(Akonadi::Collection::Id id) const
{
    RecentCollectionAction::addRecentCollection(id);
}

#include "moc_standardactionmanager.cpp"
