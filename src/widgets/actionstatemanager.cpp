/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "actionstatemanager_p.h"

#include "agentmanager.h"
#include "collectionutils.h"
#include "entitydeletedattribute.h"
#include "pastehelper_p.h"
#include "specialcollectionattribute.h"
#include "standardactionmanager.h"

#include <QApplication>
#include <QClipboard>

using namespace Akonadi;

static bool canCreateSubCollection(const Collection &collection)
{
    if (!(collection.rights() & Collection::CanCreateCollection)) {
        return false;
    }

    if (!collection.contentMimeTypes().contains(Collection::mimeType()) && !collection.contentMimeTypes().contains(Collection::virtualMimeType())) {
        return false;
    }

    return true;
}

static inline bool canContainItems(const Collection &collection)
{
    if (collection.contentMimeTypes().isEmpty()) {
        return false;
    }

    if ((collection.contentMimeTypes().count() == 1)
        && ((collection.contentMimeTypes().at(0) == Collection::mimeType()) || (collection.contentMimeTypes().at(0) == Collection::virtualMimeType()))) {
        return false;
    }

    return true;
}

void ActionStateManager::setReceiver(QObject *object)
{
    mReceiver = object;
}

void ActionStateManager::updateState(const Collection::List &collections,
                                     const Collection::List &favoriteCollections,
                                     const Item::List &items,
                                     const QList<QUrl> &clipboardUrls)
{
#ifdef QT_NO_CLIPBOARD
    Q_UNUSED(clipboardUrls);
#endif

    const int collectionCount = collections.count();
    const bool singleCollectionSelected = (collectionCount == 1);
    const bool multipleCollectionsSelected = (collectionCount > 1);
    const bool atLeastOneCollectionSelected = (singleCollectionSelected || multipleCollectionsSelected);

    const int itemCount = items.count();
    const bool singleItemSelected = (itemCount == 1);
    const bool multipleItemsSelected = (itemCount > 1);
    const bool atLeastOneItemSelected = (singleItemSelected || multipleItemsSelected);

    const bool listOfCollectionNotEmpty = !collections.isEmpty();
    bool canDeleteCollections = listOfCollectionNotEmpty;
    if (canDeleteCollections) {
        for (const Collection &collection : collections) {
            // do we have the necessary rights?
            if (!(collection.rights() & Collection::CanDeleteCollection)) {
                canDeleteCollections = false;
                break;
            }

            if (isRootCollection(collection)) {
                canDeleteCollections = false;
                break;
            }

            if (isResourceCollection(collection)) {
                canDeleteCollections = false;
                break;
            }
        }
    }

    bool canCutCollections = canDeleteCollections; // we must be able to delete for cutting
    for (const Collection &collection : collections) {
        if (isSpecialCollection(collection)) {
            canCutCollections = false;
            break;
        }

        if (!isFolderCollection(collection)) {
            canCutCollections = false;
            break;
        }
    }

    const bool canMoveCollections = canCutCollections; // we must be able to cut for moving

    bool canCopyCollections = listOfCollectionNotEmpty;
    if (canCopyCollections) {
        for (const Collection &collection : collections) {
            if (isRootCollection(collection)) {
                canCopyCollections = false;
                break;
            }

            if (!isFolderCollection(collection)) {
                canCopyCollections = false;
                break;
            }
        }
    }
    bool canAddToFavoriteCollections = listOfCollectionNotEmpty;
    if (canAddToFavoriteCollections) {
        for (const Collection &collection : collections) {
            if (isRootCollection(collection)) {
                canAddToFavoriteCollections = false;
                break;
            }

            if (isFavoriteCollection(collection)) {
                canAddToFavoriteCollections = false;
                break;
            }

            if (!isFolderCollection(collection)) {
                canAddToFavoriteCollections = false;
                break;
            }

            if (!canContainItems(collection)) {
                canAddToFavoriteCollections = false;
                break;
            }
        }
    }

    bool collectionsAreFolders = listOfCollectionNotEmpty;

    for (const Collection &collection : collections) {
        if (!isFolderCollection(collection)) {
            collectionsAreFolders = false;
            break;
        }
    }

    bool collectionsAreInTrash = false;
    for (const Collection &collection : collections) {
        if (collection.hasAttribute<EntityDeletedAttribute>()) {
            collectionsAreInTrash = true;
            break;
        }
    }

    bool atLeastOneCollectionCanHaveItems = false;
    for (const Collection &collection : collections) {
        if (collectionCanHaveItems(collection)) {
            atLeastOneCollectionCanHaveItems = true;
            break;
        }
    }
    for (const Collection &collection : favoriteCollections) {
        if (collectionCanHaveItems(collection)) {
            atLeastOneCollectionCanHaveItems = true;
            break;
        }
    }

    const Collection collection = (!collections.isEmpty() ? collections.first() : Collection());

    // collection specific actions
    enableAction(StandardActionManager::CreateCollection,
                 singleCollectionSelected && // we can create only inside one collection
                     canCreateSubCollection(collection)); // we need the necessary rights

    enableAction(StandardActionManager::DeleteCollections, canDeleteCollections);

    enableAction(StandardActionManager::CopyCollections, canCopyCollections);

    enableAction(StandardActionManager::CutCollections, canCutCollections);

    enableAction(StandardActionManager::CopyCollectionToMenu, canCopyCollections);

    enableAction(StandardActionManager::MoveCollectionToMenu, canMoveCollections);

    enableAction(StandardActionManager::MoveCollectionsToTrash, atLeastOneCollectionSelected && canMoveCollections && !collectionsAreInTrash);

    enableAction(StandardActionManager::RestoreCollectionsFromTrash, atLeastOneCollectionSelected && canMoveCollections && collectionsAreInTrash);

    enableAction(StandardActionManager::CopyCollectionToDialog, canCopyCollections);

    enableAction(StandardActionManager::MoveCollectionToDialog, canMoveCollections);

    enableAction(StandardActionManager::CollectionProperties,
                 singleCollectionSelected && // we can only configure one collection at a time
                     !isRootCollection(collection)); // we can not configure the root collection

    enableAction(StandardActionManager::SynchronizeCollections, atLeastOneCollectionCanHaveItems); // it must be a valid folder collection

    enableAction(StandardActionManager::SynchronizeCollectionsRecursive,
                 atLeastOneCollectionSelected && collectionsAreFolders); // it must be a valid folder collection
#ifndef QT_NO_CLIPBOARD
    enableAction(StandardActionManager::Paste,
                 singleCollectionSelected && // we can paste only into a single collection
                     PasteHelper::canPaste(clipboardUrls, collection, Qt::CopyAction)); // there must be data on the clipboard
#else
    enableAction(StandardActionManager::Paste, false); // no support for clipboard -> no paste
#endif

    // favorite collections specific actions
    enableAction(StandardActionManager::AddToFavoriteCollections, canAddToFavoriteCollections);

    const bool canRemoveFromFavoriteCollections = !favoriteCollections.isEmpty();
    enableAction(StandardActionManager::RemoveFromFavoriteCollections, canRemoveFromFavoriteCollections);

    enableAction(StandardActionManager::RenameFavoriteCollection, favoriteCollections.count() == 1); // we can rename only one collection at a time

    // resource specific actions
    int resourceCollectionCount = 0;
    bool canDeleteResources = true;
    bool canConfigureResource = true;
    bool canSynchronizeResources = true;
    for (const Collection &collection : collections) {
        if (isResourceCollection(collection)) {
            resourceCollectionCount++;

            // check that the 'NoConfig' flag is not set for the resource
            if (hasResourceCapability(collection, QStringLiteral("NoConfig"))) {
                canConfigureResource = false;
            }
        } else {
            // we selected a non-resource collection
            canDeleteResources = false;
            canConfigureResource = false;
            canSynchronizeResources = false;
        }
    }

    if (resourceCollectionCount == 0) {
        // not a single resource collection has been selected
        canDeleteResources = false;
        canConfigureResource = false;
        canSynchronizeResources = false;
    }

    enableAction(StandardActionManager::CreateResource, true);
    enableAction(StandardActionManager::DeleteResources, canDeleteResources);
    enableAction(StandardActionManager::ResourceProperties, canConfigureResource);
    enableAction(StandardActionManager::SynchronizeResources, canSynchronizeResources);
    enableAction(StandardActionManager::SynchronizeCollectionTree, canSynchronizeResources);

    if (collectionsAreInTrash) {
        updateAlternatingAction(StandardActionManager::MoveToTrashRestoreCollectionAlternative);
        // updatePluralLabel( StandardActionManager::MoveToTrashRestoreCollectionAlternative, collectionCount );
    } else {
        updateAlternatingAction(StandardActionManager::MoveToTrashRestoreCollection);
    }
    enableAction(StandardActionManager::MoveToTrashRestoreCollection, atLeastOneCollectionSelected && canMoveCollections);

    // item specific actions
    bool canDeleteItems = (!items.isEmpty()); // TODO: fixme
    for (const Item &item : std::as_const(items)) {
        const Collection parentCollection = item.parentCollection();
        if (!parentCollection.isValid()) {
            continue;
        }

        canDeleteItems = canDeleteItems && (parentCollection.rights() & Collection::CanDeleteItem);
    }

    bool itemsAreInTrash = false;
    for (const Item &item : std::as_const(items)) {
        if (item.hasAttribute<EntityDeletedAttribute>()) {
            itemsAreInTrash = true;
            break;
        }
    }

    enableAction(StandardActionManager::CopyItems, atLeastOneItemSelected); // we need items to work with

    enableAction(StandardActionManager::CutItems,
                 atLeastOneItemSelected && // we need items to work with
                     canDeleteItems); // we need the necessary rights

    enableAction(StandardActionManager::DeleteItems,
                 atLeastOneItemSelected && // we need items to work with
                     canDeleteItems); // we need the necessary rights

    enableAction(StandardActionManager::CopyItemToMenu, atLeastOneItemSelected); // we need items to work with

    enableAction(StandardActionManager::MoveItemToMenu,
                 atLeastOneItemSelected && // we need items to work with
                     canDeleteItems); // we need the necessary rights

    enableAction(StandardActionManager::MoveItemsToTrash, atLeastOneItemSelected && canDeleteItems && !itemsAreInTrash);

    enableAction(StandardActionManager::RestoreItemsFromTrash, atLeastOneItemSelected && itemsAreInTrash);

    enableAction(StandardActionManager::CopyItemToDialog, atLeastOneItemSelected); // we need items to work with

    enableAction(StandardActionManager::MoveItemToDialog,
                 atLeastOneItemSelected && // we need items to work with
                     canDeleteItems); // we need the necessary rights

    if (itemsAreInTrash) {
        updateAlternatingAction(StandardActionManager::MoveToTrashRestoreItemAlternative);
        // updatePluralLabel( StandardActionManager::MoveToTrashRestoreItemAlternative, itemCount );
    } else {
        updateAlternatingAction(StandardActionManager::MoveToTrashRestoreItem);
    }
    enableAction(StandardActionManager::MoveToTrashRestoreItem,
                 atLeastOneItemSelected && // we need items to work with
                     canDeleteItems); // we need the necessary rights

    // update the texts of the actions
    updatePluralLabel(StandardActionManager::CopyCollections, collectionCount);
    updatePluralLabel(StandardActionManager::CopyItems, itemCount);
    updatePluralLabel(StandardActionManager::DeleteItems, itemCount);
    updatePluralLabel(StandardActionManager::CutItems, itemCount);
    updatePluralLabel(StandardActionManager::CutCollections, collectionCount);
    updatePluralLabel(StandardActionManager::DeleteCollections, collectionCount);
    updatePluralLabel(StandardActionManager::SynchronizeCollections, collectionCount);
    updatePluralLabel(StandardActionManager::SynchronizeCollectionsRecursive, collectionCount);
    updatePluralLabel(StandardActionManager::DeleteResources, resourceCollectionCount);
    updatePluralLabel(StandardActionManager::SynchronizeResources, resourceCollectionCount);
    updatePluralLabel(StandardActionManager::SynchronizeCollectionTree, resourceCollectionCount);
}

bool ActionStateManager::isRootCollection(const Collection &collection) const
{
    return CollectionUtils::isRoot(collection);
}

bool ActionStateManager::isResourceCollection(const Collection &collection) const
{
    return CollectionUtils::isResource(collection);
}

bool ActionStateManager::isFolderCollection(const Collection &collection) const
{
    return (CollectionUtils::isFolder(collection) || CollectionUtils::isResource(collection) || CollectionUtils::isStructural(collection));
}

bool ActionStateManager::isSpecialCollection(const Collection &collection) const
{
    return collection.hasAttribute<SpecialCollectionAttribute>();
}

bool ActionStateManager::isFavoriteCollection(const Collection &collection) const
{
    if (!mReceiver) {
        return false;
    }

    bool result = false;
    QMetaObject::invokeMethod(mReceiver, "isFavoriteCollection", Qt::DirectConnection, Q_RETURN_ARG(bool, result), Q_ARG(Akonadi::Collection, collection));

    return result;
}

bool ActionStateManager::hasResourceCapability(const Collection &collection, const QString &capability) const
{
    const Akonadi::AgentInstance instance = AgentManager::self()->instance(collection.resource());

    return instance.type().capabilities().contains(capability);
}

bool ActionStateManager::collectionCanHaveItems(const Collection &collection) const
{
    return !(collection.contentMimeTypes() == (QStringList() << QStringLiteral("inode/directory")) || CollectionUtils::isStructural(collection));
}

void ActionStateManager::enableAction(int action, bool state)
{
    if (!mReceiver) {
        return;
    }

    QMetaObject::invokeMethod(mReceiver, "enableAction", Qt::DirectConnection, Q_ARG(int, action), Q_ARG(bool, state));
}

void ActionStateManager::updatePluralLabel(int action, int count)
{
    if (!mReceiver) {
        return;
    }

    QMetaObject::invokeMethod(mReceiver, "updatePluralLabel", Qt::DirectConnection, Q_ARG(int, action), Q_ARG(int, count));
}

void ActionStateManager::updateAlternatingAction(int action)
{
    if (!mReceiver) {
        return;
    }

    QMetaObject::invokeMethod(mReceiver, "updateAlternatingAction", Qt::DirectConnection, Q_ARG(int, action));
}
