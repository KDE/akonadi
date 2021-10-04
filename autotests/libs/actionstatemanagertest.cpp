/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collection.h"
#include <qtest_akonadi.h>

#include "../src/widgets/actionstatemanager_p.h"
#include "../src/widgets/standardactionmanager.h"

#define QT_NO_CLIPBOARD // allow running without GUI
#include "../src/widgets/actionstatemanager.cpp"
#undef QT_NO_CLIPBOARD

using StateMap = QHash<Akonadi::StandardActionManager::Type, bool>;
Q_DECLARE_METATYPE(StateMap)

using namespace Akonadi;

class ActionStateManagerTest;

class UnitActionStateManager : public ActionStateManager
{
public:
    explicit UnitActionStateManager(ActionStateManagerTest *receiver);

protected:
    bool hasResourceCapability(const Collection &collection, const QString &capability) const override;

private:
    ActionStateManagerTest *mReceiver = nullptr;
};

class ActionStateManagerTest : public QObject
{
    Q_OBJECT

public:
    ActionStateManagerTest()
        : rootCollection(Collection::root())
    {
        const QString dummyMimeType(QStringLiteral("text/dummy"));

        resourceCollectionOne.setId(1);
        resourceCollectionOne.setName(QStringLiteral("resourceCollectionOne"));
        resourceCollectionOne.setRights(Collection::ReadOnly);
        resourceCollectionOne.setParentCollection(rootCollection);
        resourceCollectionOne.setContentMimeTypes(QStringList() << Collection::mimeType() << dummyMimeType);

        folderCollectionOne.setId(10);
        folderCollectionOne.setName(QStringLiteral("folderCollectionOne"));
        folderCollectionOne.setRights(Collection::ReadOnly);
        folderCollectionOne.setParentCollection(resourceCollectionOne);
        folderCollectionOne.setContentMimeTypes(QStringList() << Collection::mimeType() << dummyMimeType);

        resourceCollectionTwo.setId(2);
        resourceCollectionTwo.setName(QStringLiteral("resourceCollectionTwo"));
        resourceCollectionTwo.setRights(Collection::AllRights);
        resourceCollectionTwo.setParentCollection(rootCollection);
        resourceCollectionTwo.setContentMimeTypes(QStringList() << Collection::mimeType() << dummyMimeType);

        folderCollectionTwo.setId(20);
        folderCollectionTwo.setName(QStringLiteral("folderCollectionTwo"));
        folderCollectionTwo.setRights(Collection::AllRights);
        folderCollectionTwo.setParentCollection(resourceCollectionTwo);
        folderCollectionTwo.setContentMimeTypes(QStringList() << Collection::mimeType() << dummyMimeType);

        resourceCollectionThree.setId(3);
        resourceCollectionThree.setName(QStringLiteral("resourceCollectionThree"));
        resourceCollectionThree.setRights(Collection::AllRights);
        resourceCollectionThree.setParentCollection(rootCollection);
        resourceCollectionThree.setContentMimeTypes(QStringList() << Collection::mimeType() << dummyMimeType);

        folderCollectionThree.setId(30);
        folderCollectionThree.setName(QStringLiteral("folderCollectionThree"));
        folderCollectionThree.setRights(Collection::AllRights);
        folderCollectionThree.setParentCollection(resourceCollectionThree);
        folderCollectionThree.setContentMimeTypes(QStringList() << Collection::mimeType() << dummyMimeType);

        folderCollectionThree.setId(31);
        folderCollectionThree.setName(QStringLiteral("folderCollectionThreeOne"));
        folderCollectionThree.setRights(Collection::AllRights);
        folderCollectionThree.setParentCollection(resourceCollectionThree);

        mCapabilityMap.insert(QStringLiteral("NoConfig"), Collection::List() << resourceCollectionThree);
        mFavoriteCollectionMap.insert(folderCollectionThree.id());
    }

    bool hasResourceCapability(const Collection &collection, const QString &capability) const
    {
        return mCapabilityMap.value(capability).contains(collection);
    }

public Q_SLOTS:
    void enableAction(int type, bool enable)
    {
        mStateMap.insert(static_cast<StandardActionManager::Type>(type), enable);
    }

    void updatePluralLabel(int type, int count)
    {
        Q_UNUSED(type)
        Q_UNUSED(count)
    }

    bool isFavoriteCollection(const Akonadi::Collection &collection)
    {
        return mFavoriteCollectionMap.contains(collection.id());
    }

    void updateAlternatingAction(int action)
    {
        Q_UNUSED(action)
    }

private Q_SLOTS:

    void init()
    {
        mStateMap.clear();
    }

    void testCollectionSelected_data()
    {
        QTest::addColumn<Collection::List>("collections");
        QTest::addColumn<StateMap>("stateMap");

        {
            Collection::List collectionList;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, false);
            map.insert(StandardActionManager::CopyCollections, false);
            map.insert(StandardActionManager::DeleteCollections, false);
            map.insert(StandardActionManager::SynchronizeCollections, false);
            map.insert(StandardActionManager::CollectionProperties, false);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, false);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, false);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, false);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, false);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, false);
            map.insert(StandardActionManager::ResourceProperties, false);
            map.insert(StandardActionManager::SynchronizeResources, false);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, false);
            map.insert(StandardActionManager::MoveCollectionToDialog, false);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, false);
            map.insert(StandardActionManager::MoveCollectionsToTrash, false);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, false);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, false);

            QTest::newRow("nothing selected") << collectionList << map;
        }

        {
            Collection::List collectionList;
            collectionList << rootCollection;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, false);
            map.insert(StandardActionManager::CopyCollections, false);
            map.insert(StandardActionManager::DeleteCollections, false);
            map.insert(StandardActionManager::SynchronizeCollections, false);
            map.insert(StandardActionManager::CollectionProperties, false);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, false);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, false);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, false);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, false);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, false);
            map.insert(StandardActionManager::ResourceProperties, false);
            map.insert(StandardActionManager::SynchronizeResources, false);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, false);
            map.insert(StandardActionManager::MoveCollectionToDialog, false);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, false);
            map.insert(StandardActionManager::MoveCollectionsToTrash, false);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, false);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, false);

            QTest::newRow("root collection selected") << collectionList << map;
        }

        {
            Collection::List collectionList;
            collectionList << resourceCollectionOne;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, false);
            map.insert(StandardActionManager::CopyCollections, true);
            map.insert(StandardActionManager::DeleteCollections, false);
            map.insert(StandardActionManager::SynchronizeCollections, true);
            map.insert(StandardActionManager::CollectionProperties, true);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, true);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, true);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, false);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, false);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, true);
            map.insert(StandardActionManager::ResourceProperties, true);
            map.insert(StandardActionManager::SynchronizeResources, true);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, true);
            map.insert(StandardActionManager::MoveCollectionToDialog, false);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, true);
            map.insert(StandardActionManager::MoveCollectionsToTrash, false);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, false);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, true);

            QTest::newRow("read-only resource collection selected") << collectionList << map;
        }

        {
            Collection::List collectionList;
            collectionList << resourceCollectionTwo;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, true);
            map.insert(StandardActionManager::CopyCollections, true);
            map.insert(StandardActionManager::DeleteCollections, false);
            map.insert(StandardActionManager::SynchronizeCollections, true);
            map.insert(StandardActionManager::CollectionProperties, true);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, true);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, true);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, false);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, false);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, true);
            map.insert(StandardActionManager::ResourceProperties, true);
            map.insert(StandardActionManager::SynchronizeResources, true);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, true);
            map.insert(StandardActionManager::MoveCollectionToDialog, false);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, true);
            map.insert(StandardActionManager::MoveCollectionsToTrash, false);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, false);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, true);

            QTest::newRow("writable resource collection selected") << collectionList << map;
        }

        {
            Collection::List collectionList;
            collectionList << resourceCollectionThree;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, true);
            map.insert(StandardActionManager::CopyCollections, true);
            map.insert(StandardActionManager::DeleteCollections, false);
            map.insert(StandardActionManager::SynchronizeCollections, true);
            map.insert(StandardActionManager::CollectionProperties, true);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, true);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, true);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, false);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, false);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, true);
            map.insert(StandardActionManager::ResourceProperties, false);
            map.insert(StandardActionManager::SynchronizeResources, true);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, true);
            map.insert(StandardActionManager::MoveCollectionToDialog, false);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, true);
            map.insert(StandardActionManager::MoveCollectionsToTrash, false);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, false);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, true);

            QTest::newRow("non-configurable resource collection selected") << collectionList << map;
        }

        {
            Collection::List collectionList;
            collectionList << folderCollectionOne;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, false);
            map.insert(StandardActionManager::CopyCollections, true);
            map.insert(StandardActionManager::DeleteCollections, false);
            map.insert(StandardActionManager::SynchronizeCollections, true);
            map.insert(StandardActionManager::CollectionProperties, true);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, true);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, true);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, false);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, false);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, false);
            map.insert(StandardActionManager::ResourceProperties, false);
            map.insert(StandardActionManager::SynchronizeResources, false);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, true);
            map.insert(StandardActionManager::MoveCollectionToDialog, false);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, true);
            map.insert(StandardActionManager::MoveCollectionsToTrash, false);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, false);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, false);

            QTest::newRow("read-only folder collection selected") << collectionList << map;
        }

        {
            Collection::List collectionList;
            collectionList << folderCollectionTwo;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, true);
            map.insert(StandardActionManager::CopyCollections, true);
            map.insert(StandardActionManager::DeleteCollections, true);
            map.insert(StandardActionManager::SynchronizeCollections, true);
            map.insert(StandardActionManager::CollectionProperties, true);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, true);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, true);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, true);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, true);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, false);
            map.insert(StandardActionManager::ResourceProperties, false);
            map.insert(StandardActionManager::SynchronizeResources, false);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, true);
            map.insert(StandardActionManager::MoveCollectionToDialog, true);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, true);
            map.insert(StandardActionManager::MoveCollectionsToTrash, true);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, true);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, false);

            QTest::newRow("writable folder collection selected") << collectionList << map;
        }

        {
            Collection::List collectionList;
            collectionList << folderCollectionThree;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, true);
            map.insert(StandardActionManager::CopyCollections, true);
            map.insert(StandardActionManager::DeleteCollections, true);
            map.insert(StandardActionManager::SynchronizeCollections, true);
            map.insert(StandardActionManager::CollectionProperties, true);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, false);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, true);
            map.insert(StandardActionManager::RenameFavoriteCollection, true);
            map.insert(StandardActionManager::CopyCollectionToMenu, true);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, true);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, true);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, false);
            map.insert(StandardActionManager::ResourceProperties, false);
            map.insert(StandardActionManager::SynchronizeResources, false);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, true);
            map.insert(StandardActionManager::MoveCollectionToDialog, true);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, true);
            map.insert(StandardActionManager::MoveCollectionsToTrash, true);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, true);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, false);

            QTest::newRow("favorite writable folder collection selected") << collectionList << map;
        }

        {
            Collection::List collectionList;
            collectionList << folderCollectionThreeOne;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, false); // content mimetype is missing
            map.insert(StandardActionManager::CopyCollections, true);
            map.insert(StandardActionManager::DeleteCollections, true);
            map.insert(StandardActionManager::SynchronizeCollections, false);
            map.insert(StandardActionManager::CollectionProperties, true);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, false); // content mimetype is missing
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, true);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, true);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, true);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, false);
            map.insert(StandardActionManager::ResourceProperties, false);
            map.insert(StandardActionManager::SynchronizeResources, false);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, true);
            map.insert(StandardActionManager::MoveCollectionToDialog, true);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, true);
            map.insert(StandardActionManager::MoveCollectionsToTrash, true);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, true);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, false);

            QTest::newRow("structural folder collection selected") << collectionList << map;
        }

        // multiple collections
        {
            Collection::List collectionList;
            collectionList << rootCollection << resourceCollectionTwo;

            StateMap map;
            map.insert(StandardActionManager::CreateCollection, false);
            map.insert(StandardActionManager::CopyCollections, false);
            map.insert(StandardActionManager::DeleteCollections, false);
            map.insert(StandardActionManager::SynchronizeCollections, true);
            map.insert(StandardActionManager::CollectionProperties, false);
            map.insert(StandardActionManager::CopyItems, false);
            map.insert(StandardActionManager::Paste, false);
            map.insert(StandardActionManager::DeleteItems, false);
            map.insert(StandardActionManager::AddToFavoriteCollections, false);
            map.insert(StandardActionManager::RemoveFromFavoriteCollections, false);
            map.insert(StandardActionManager::RenameFavoriteCollection, false);
            map.insert(StandardActionManager::CopyCollectionToMenu, false);
            map.insert(StandardActionManager::CopyItemToMenu, false);
            map.insert(StandardActionManager::MoveItemToMenu, false);
            map.insert(StandardActionManager::MoveCollectionToMenu, false);
            map.insert(StandardActionManager::CutItems, false);
            map.insert(StandardActionManager::CutCollections, false);
            map.insert(StandardActionManager::CreateResource, true);
            map.insert(StandardActionManager::DeleteResources, false);
            map.insert(StandardActionManager::ResourceProperties, false);
            map.insert(StandardActionManager::SynchronizeResources, false);
            map.insert(StandardActionManager::MoveItemToDialog, false);
            map.insert(StandardActionManager::CopyItemToDialog, false);
            map.insert(StandardActionManager::CopyCollectionToDialog, false);
            map.insert(StandardActionManager::MoveCollectionToDialog, false);
            map.insert(StandardActionManager::SynchronizeCollectionsRecursive, false);
            map.insert(StandardActionManager::MoveCollectionsToTrash, false);
            map.insert(StandardActionManager::MoveItemsToTrash, false);
            map.insert(StandardActionManager::RestoreCollectionsFromTrash, false);
            map.insert(StandardActionManager::RestoreItemsFromTrash, false);
            map.insert(StandardActionManager::MoveToTrashRestoreCollection, false);
            map.insert(StandardActionManager::MoveToTrashRestoreItem, false);
            map.insert(StandardActionManager::SynchronizeCollectionTree, false);

            QTest::newRow("root collection and writable resource collection selected") << collectionList << map;
        }
    }

    void testCollectionSelected()
    {
        QFETCH(Collection::List, collections);
        QFETCH(StateMap, stateMap);

        UnitActionStateManager manager(this);
        Collection::List favoriteCollections;
        if (collections.contains(folderCollectionThree)) {
            favoriteCollections << folderCollectionThree;
        }

        manager.updateState(collections, favoriteCollections, Item::List());

        QCOMPARE(stateMap.count(), mStateMap.count());

        QHashIterator<StandardActionManager::Type, bool> it(stateMap);
        while (it.hasNext()) {
            it.next();
            // qDebug() << it.key();
            QVERIFY(mStateMap.contains(it.key()));
            const bool expected = mStateMap.value(it.key());
            if (it.value() != expected) {
                qWarning() << "Wrong state for" << it.key();
            }
            QCOMPARE(it.value(), expected);
        }
    }

private:
    /**
     * The structure of our fake collections:
     *
     * rootCollection
     *  |
     *  +- resourceCollectionOne
     *  |   |
     *  |   `folderCollectionOne
     *  |
     *  +- resourceCollectionTwo
     *  |   |
     *  |   `folderCollectionTwo
     *  |
     *  `- resourceCollectionThree
     *      |
     *      +-folderCollectionThree
     *      |
     *      `-folderCollectionThreeOne
     */
    Collection rootCollection;
    Collection resourceCollectionOne;
    Collection resourceCollectionTwo;
    Collection resourceCollectionThree;
    Collection folderCollectionOne;
    Collection folderCollectionTwo;
    Collection folderCollectionThree;
    Collection folderCollectionThreeOne;

    StateMap mStateMap;
    QHash<QString, Collection::List> mCapabilityMap;
    QSet<qlonglong> mFavoriteCollectionMap;
};

UnitActionStateManager::UnitActionStateManager(ActionStateManagerTest *receiver)
    : mReceiver(receiver)
{
    setReceiver(receiver);
}

bool UnitActionStateManager::hasResourceCapability(const Collection &collection, const QString &capability) const
{
    return mReceiver->hasResourceCapability(collection, capability);
}

QTEST_AKONADIMAIN(ActionStateManagerTest)

#include "actionstatemanagertest.moc"
