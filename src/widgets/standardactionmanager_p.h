/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentinstance.h"
#include "recentcollectionaction_p.h"
#include "standardactionmanager.h"

#include "actionstatemanager_p.h"

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QTimer>

namespace Akonadi
{

class StandardActionManagerPrivate
{
public:
    explicit StandardActionManagerPrivate(StandardActionManager *parent);

    void enableAction(int type, bool enable); // private slot, called by ActionStateManager
    void enableAction(StandardActionManager::Type type, bool enable);

    void aboutToShowMenu();
    void createActionFolderMenu(QMenu *menu, StandardActionManager::Type type);

    void updateAlternatingAction(int type); // private slot, called by ActionStateManager
    void updateAlternatingAction(StandardActionManager::Type type);

    void updatePluralLabel(int type, int count); // private slot, called by ActionStateManager
    void updatePluralLabel(StandardActionManager::Type type, int count); // private slot, called by ActionStateManager

    bool isFavoriteCollection(const Akonadi::Collection &collection) const; // private slot, called by ActionStateManager

    void encodeToClipboard(QItemSelectionModel *selectionModel, bool cut = false);

    static Akonadi::Collection::List collectionsForIndexes(const QModelIndexList &list);

    void delayedUpdateActions();
    void updateActions();

#ifndef QT_NO_CLIPBOARD
    void clipboardChanged();
#endif

    QItemSelection mapToEntityTreeModel(const QAbstractItemModel *model, const QItemSelection &selection) const;
    QItemSelection mapFromEntityTreeModel(const QAbstractItemModel *model, const QItemSelection &selection) const;

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

    void collectionSelectionChanged();
    void favoriteSelectionChanged();

    void slotCreateCollection();
    void slotCopyCollections();
    void slotCutCollections();

    Collection::List selectedCollections();

    void slotDeleteCollection();
    void slotMoveCollectionToTrash();
    void slotRestoreCollectionFromTrash();

    Item::List selectedItems() const;

    void slotMoveItemToTrash();
    void slotRestoreItemFromTrash();
    void slotTrashRestoreCollection();
    void slotTrashRestoreItem();
    void slotSynchronizeCollection();

    bool testAndSetOnlineResources(const Akonadi::Collection &collection);

    void slotSynchronizeCollectionRecursive();
    void slotCollectionProperties() const;
    void slotCopyItems();
    void slotCutItems();
    void slotPaste();
    void slotDeleteItems();
    void slotDeleteItemsDeferred(const Akonadi::Item::List &items);
    void slotLocalSubscription() const;
    void slotAddToFavorites();
    void slotRemoveFromFavorites();
    void slotRenameFavorite();
    void slotSynchronizeFavoriteCollections();

    void slotCopyCollectionTo();
    void slotCopyItemTo();
    void slotMoveCollectionTo();
    void slotMoveItemTo();
    void slotCopyCollectionTo(QAction *action);
    void slotCopyItemTo(QAction *action);
    void slotMoveCollectionTo(QAction *action);
    void slotMoveItemTo(QAction *action);

    AgentInstance::List selectedAgentInstances() const;
    AgentInstance selectedAgentInstance() const;

    void slotCreateResource();
    void slotDeleteResource() const;
    void slotSynchronizeResource() const;
    void slotSynchronizeCollectionTree() const;
    void slotResourceProperties() const;
    void slotToggleWorkOffline(bool offline);

    void pasteTo(QItemSelectionModel *selectionModel, const QAbstractItemModel *model, StandardActionManager::Type type, Qt::DropAction dropAction);
    void pasteTo(QItemSelectionModel *selectionModel, QAction *action, Qt::DropAction dropAction);

    void addRecentCollection(Akonadi::Collection::Id id) const;

    void collectionCreationResult(KJob *job) const;
    void collectionDeletionResult(KJob *job) const;
    void moveCollectionToTrashResult(KJob *job) const;
    void moveItemToTrashResult(KJob *job) const;
    void itemDeletionResult(KJob *job) const;
    void resourceCreationResult(KJob *job) const;
    void pasteResult(KJob *job) const;

    /**
     * Returns a set of mime types of the entities that are currently selected.
     */
    QSet<QString> mimeTypesOfSelection(StandardActionManager::Type type) const;

    /**
     * Returns whether items with the given @p mimeTypes can be written to the given @p collection.
     */
    bool isWritableTargetCollectionForMimeTypes(const Collection &collection, const QSet<QString> &mimeTypes, StandardActionManager::Type type) const;

    void fillFoldersMenu(const Akonadi::Collection::List &selectedCollectionsList,
                         const QSet<QString> &mimeTypes,
                         StandardActionManager::Type type,
                         QMenu *menu,
                         const QAbstractItemModel *model,
                         const QModelIndex &parentIndex);

    void checkModelsConsistency() const;

    void markCutAction(QMimeData *mimeData, bool cut) const;
    bool isCutAction(const QMimeData *mimeData) const;

    void setContextText(StandardActionManager::Type type, StandardActionManager::TextContext context, const KLocalizedString &data);
    QString contextText(StandardActionManager::Type type, StandardActionManager::TextContext context) const;
    QString contextText(StandardActionManager::Type type, StandardActionManager::TextContext context, const QString &value) const;
    QString contextText(StandardActionManager::Type type, StandardActionManager::TextContext context, int count, const QString &value) const;

    StandardActionManager *const q;
    KActionCollection *actionCollection = nullptr;
    QWidget *parentWidget = nullptr;
    QItemSelectionModel *collectionSelectionModel = nullptr;
    QItemSelectionModel *itemSelectionModel = nullptr;
    FavoriteCollectionsModel *favoritesModel = nullptr;
    QItemSelectionModel *favoriteSelectionModel = nullptr;
    bool insideSelectionSlot = false;
    QList<QAction *> actions;
    QHash<StandardActionManager::Type, KLocalizedString> pluralLabels;
    QHash<StandardActionManager::Type, KLocalizedString> pluralIconLabels;
    QTimer mDelayedUpdateTimer;

    using ContextTexts = QHash<StandardActionManager::TextContext, KLocalizedString>;
    QHash<StandardActionManager::Type, ContextTexts> contextTexts;

    ActionStateManager mActionStateManager;

    QStringList mMimeTypeFilter;
    QStringList mCapabilityFilter;
    QStringList mCollectionPropertiesPageNames;
    QMap<StandardActionManager::Type, QPointer<RecentCollectionAction>> mRecentCollectionsMenu;
#ifndef QT_NO_CLIPBOARD
    QList<QUrl> clipboardUrls;
#endif
};

}
