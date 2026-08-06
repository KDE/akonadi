/*
    SPDX-FileCopyrightText: 2026 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "etmviewstatesaver.h"
#include "entitytreemodel.h"

#include <KConfig>
#include <KConfigGroup>

#include <QItemSelectionModel>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <QTest>

using namespace Akonadi;

// Exposes the protected serialization methods for testing.
class TestStateSaver : public ETMViewStateSaver
{
public:
    using ETMViewStateSaver::indexFromConfigString;
    using ETMViewStateSaver::indexToConfigString;
};

class ETMViewStateSaverTest : public QObject
{
    Q_OBJECT

private:
    // Builds a QStandardItem carrying a Collection in EntityTreeModel::CollectionRole.
    static QStandardItem *collectionItem(Collection::Id id, const QString &resource, const QString &remoteId)
    {
        Collection col(id);
        col.setResource(resource);
        col.setRemoteId(remoteId);
        auto *item = new QStandardItem(remoteId.isEmpty() ? resource : remoteId);
        item->setData(QVariant::fromValue(col), EntityTreeModel::CollectionRole);
        return item;
    }

private Q_SLOTS:
    void shouldWriteStableRemotePathKey()
    {
        QStandardItemModel model;
        // res1 root -> calendar
        auto *root = collectionItem(1, QStringLiteral("res1"), QStringLiteral("res1root"));
        auto *calendar = collectionItem(2, QStringLiteral("res1"), QStringLiteral("https://s/cal/work/"));
        root->appendRow(calendar);
        model.appendRow(root);

        TestStateSaver saver;
        saver.setKeyFormat(ETMViewStateSaver::RemotePathKeys);

        // The '/' inside the remoteId is escaped; the resource-root remoteId is not part of the key.
        QCOMPARE(saver.indexToConfigString(calendar->index()), QStringLiteral("rres1/https:%2F%2Fs%2Fcal%2Fwork%2F"));
    }

    void shouldBuildStableKeyFromCollectionChain()
    {
        // The Collection-based helper (no model) must produce the same key as the index-based one,
        // walking parentCollection() instead of model indexes.
        Collection root(1);
        root.setResource(QStringLiteral("res1"));
        root.setRemoteId(QStringLiteral("res1root"));
        root.setParentCollection(Collection::root());

        Collection calendar(2);
        calendar.setResource(QStringLiteral("res1"));
        calendar.setRemoteId(QStringLiteral("https://s/cal/work/"));
        calendar.setParentCollection(root);

        QCOMPARE(EntityTreeModel::stableKeyForCollection(calendar), QStringLiteral("rres1/https:%2F%2Fs%2Fcal%2Fwork%2F"));

        // A single-collection resource (the root itself) is keyed by the resource anchor alone.
        QCOMPARE(EntityTreeModel::stableKeyForCollection(root), QStringLiteral("rres1/"));

        // A collection with no resource has no stable key.
        Collection noResource(3);
        noResource.setRemoteId(QStringLiteral("x"));
        QVERIFY(EntityTreeModel::stableKeyForCollection(noResource).isEmpty());
    }

    void shouldMatchIndexAndCollectionKeys()
    {
        // The index-based and Collection-based helpers must agree for the same collection.
        Collection root(1);
        root.setResource(QStringLiteral("res1"));
        root.setRemoteId(QStringLiteral("res1root"));
        root.setParentCollection(Collection::root());

        Collection mid(2);
        mid.setResource(QStringLiteral("res1"));
        mid.setRemoteId(QStringLiteral("home"));
        mid.setParentCollection(root);

        Collection leaf(3);
        leaf.setResource(QStringLiteral("res1"));
        leaf.setRemoteId(QStringLiteral("2026"));
        leaf.setParentCollection(mid);

        QStandardItemModel model;
        auto *rootItem = collectionItem(1, QStringLiteral("res1"), QStringLiteral("res1root"));
        auto *midItem = collectionItem(2, QStringLiteral("res1"), QStringLiteral("home"));
        auto *leafItem = collectionItem(3, QStringLiteral("res1"), QStringLiteral("2026"));
        midItem->appendRow(leafItem);
        rootItem->appendRow(midItem);
        model.appendRow(rootItem);

        QCOMPARE(EntityTreeModel::stableKeyForCollection(leaf), EntityTreeModel::stableKeyForCollectionIndex(leafItem->index()));
        QCOMPARE(EntityTreeModel::stableKeyForCollection(leaf), QStringLiteral("rres1/home/2026"));
    }

    void shouldRoundTripStableRemotePathKey()
    {
        QStandardItemModel model;
        auto *root = collectionItem(1, QStringLiteral("res1"), QStringLiteral("res1root"));
        auto *work = collectionItem(2, QStringLiteral("res1"), QStringLiteral("work"));
        auto *home = collectionItem(3, QStringLiteral("res1"), QStringLiteral("home"));
        root->appendRow(work);
        root->appendRow(home);
        model.appendRow(root);

        TestStateSaver saver;
        saver.setKeyFormat(ETMViewStateSaver::RemotePathKeys);

        const QString key = saver.indexToConfigString(home->index());
        QCOMPARE(key, QStringLiteral("rres1/home"));
        QCOMPARE(saver.indexFromConfigString(&model, key), home->index());
    }

    void shouldKeySingleCollectionResourceByResourceAlone()
    {
        // A resource whose only collection is the root itself (e.g. a single-file iCal calendar).
        QStandardItemModel model;
        auto *root = collectionItem(5, QStringLiteral("res2"), QStringLiteral("res2root"));
        model.appendRow(root);

        TestStateSaver saver;
        saver.setKeyFormat(ETMViewStateSaver::RemotePathKeys);

        const QString key = saver.indexToConfigString(root->index());
        QCOMPARE(key, QStringLiteral("rres2/"));
        QCOMPARE(saver.indexFromConfigString(&model, key), root->index());
    }

    void shouldFallBackToIdKeyWhenRemoteIdMissing()
    {
        // A pathless collection (empty remoteId) has no stable path, so the id key is used.
        QStandardItemModel model;
        auto *root = collectionItem(1, QStringLiteral("res1"), QStringLiteral("res1root"));
        auto *pathless = collectionItem(7, QStringLiteral("res1"), QString());
        root->appendRow(pathless);
        model.appendRow(root);

        TestStateSaver saver;
        saver.setKeyFormat(ETMViewStateSaver::RemotePathKeys);

        QCOMPARE(saver.indexToConfigString(pathless->index()), QStringLiteral("c7"));
    }

    void shouldNotResolveUnknownStableKey()
    {
        QStandardItemModel model;
        auto *root = collectionItem(1, QStringLiteral("res1"), QStringLiteral("res1root"));
        model.appendRow(root);

        TestStateSaver saver;
        QVERIFY(!saver.indexFromConfigString(&model, QStringLiteral("rres1/gone")).isValid());
    }

    void shouldSaveSelectionAsRemotePathKeys()
    {
        // Drives the real public API: a checked calendar must land in the config as an "r" key.
        QStandardItemModel model;
        auto *root = collectionItem(1, QStringLiteral("res1"), QStringLiteral("res1root"));
        auto *work = collectionItem(2, QStringLiteral("res1"), QStringLiteral("https://s/cal/work/"));
        root->appendRow(work);
        model.appendRow(root);

        QItemSelectionModel selectionModel(&model);
        selectionModel.select(work->index(), QItemSelectionModel::Select);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        KConfig config(dir.filePath(QStringLiteral("testrc")));
        KConfigGroup group = config.group(QStringLiteral("GlobalCollectionSelection"));

        ETMViewStateSaver saver;
        saver.setKeyFormat(ETMViewStateSaver::RemotePathKeys);
        saver.setSelectionModel(&selectionModel);
        saver.saveState(group);

        QCOMPARE(group.readEntry("Selection", QStringList()), QStringList{QStringLiteral("rres1/https:%2F%2Fs%2Fcal%2Fwork%2F")});
    }

    void shouldRestoreSelectionFromRemotePathKeys()
    {
        // The other half of the migration: an already-migrated config must check the same calendar again,
        // even though the collection ids are completely different from the ones saved earlier.
        QStandardItemModel model;
        auto *root = collectionItem(101, QStringLiteral("res1"), QStringLiteral("res1root"));
        auto *work = collectionItem(102, QStringLiteral("res1"), QStringLiteral("https://s/cal/work/"));
        auto *home = collectionItem(103, QStringLiteral("res1"), QStringLiteral("https://s/cal/home/"));
        root->appendRow(work);
        root->appendRow(home);
        model.appendRow(root);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        KConfig config(dir.filePath(QStringLiteral("testrc")));
        KConfigGroup group = config.group(QStringLiteral("GlobalCollectionSelection"));
        group.writeEntry("Selection", QStringList{QStringLiteral("rres1/https:%2F%2Fs%2Fcal%2Fwork%2F")});

        QItemSelectionModel selectionModel(&model);
        ETMViewStateSaver saver;
        saver.setSelectionModel(&selectionModel);
        saver.restoreState(group);

        QCOMPARE(selectionModel.selectedIndexes(), QModelIndexList{work->index()});
    }

    // Reproduces the shutdown race: KOrganizer's queryClose() saves unconditionally, but the
    // restore is asynchronous. If the collections are not in the model yet, the selection model
    // is still empty and saving overwrites the stored keys with nothing.
    void shouldNotLoseUnresolvedKeysOnSave()
    {
        QStandardItemModel model; // deliberately empty: the resource has not populated yet
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        KConfig config(dir.filePath(QStringLiteral("testrc")));
        KConfigGroup group = config.group(QStringLiteral("GlobalCollectionSelection"));
        const QStringList stored{QStringLiteral("rres1/https:%2F%2Fs%2Fcal%2Fwork%2F")};
        group.writeEntry("Selection", stored);

        QItemSelectionModel selectionModel(&model);
        ETMViewStateSaver restorer;
        restorer.setSelectionModel(&selectionModel);
        restorer.restoreState(group); // key stays pending, nothing gets selected
        QVERIFY(selectionModel.selectedIndexes().isEmpty());

        // Now the user quits before the model populated.
        ETMViewStateSaver saver;
        saver.setKeyFormat(ETMViewStateSaver::RemotePathKeys);
        saver.setSelectionModel(&selectionModel);
        saver.saveState(group);

        QCOMPARE(group.readEntry("Selection", QStringList()), stored);
    }

    void shouldStillDropDeselectedKeys()
    {
        // The safety net must not resurrect a calendar the user deliberately unchecked:
        // its collection *is* in the model, it is just no longer selected.
        QStandardItemModel model;
        auto *root = collectionItem(1, QStringLiteral("res1"), QStringLiteral("res1root"));
        auto *work = collectionItem(2, QStringLiteral("res1"), QStringLiteral("work"));
        root->appendRow(work);
        model.appendRow(root);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        KConfig config(dir.filePath(QStringLiteral("testrc")));
        KConfigGroup group = config.group(QStringLiteral("GlobalCollectionSelection"));
        group.writeEntry("Selection", QStringList{QStringLiteral("rres1/work")});

        QItemSelectionModel selectionModel(&model); // nothing selected: the user unchecked it
        ETMViewStateSaver saver;
        saver.setKeyFormat(ETMViewStateSaver::RemotePathKeys);
        saver.setSelectionModel(&selectionModel);
        saver.saveState(group);

        QVERIFY(group.readEntry("Selection", QStringList()).isEmpty());
    }

    void shouldNotKeepMissingCollectionsForIdKeys()
    {
        // In the default id format, saveState() must behave exactly like the base class: a stored
        // key whose collection is missing is dropped, because a stale id can resolve to a different
        // collection later.
        QStandardItemModel model; // empty: the stored collection is not present
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        KConfig config(dir.filePath(QStringLiteral("testrc")));
        KConfigGroup group = config.group(QStringLiteral("GlobalCollectionSelection"));
        group.writeEntry("Selection", QStringList{QStringLiteral("c5")});

        QItemSelectionModel selectionModel(&model);
        ETMViewStateSaver saver; // default IdKeys
        saver.setSelectionModel(&selectionModel);
        saver.saveState(group);

        QVERIFY(group.readEntry("Selection", QStringList()).isEmpty());
    }

    void shouldSurviveKConfigRoundTrip()
    {
        // KConfigViewStateSaver stores the keys as a QStringList under "Selection".
        // The stable keys contain '/', '\' and can contain ',', so make sure KConfig's
        // escaping gives them back unchanged.
        const QStringList keys{
            QStringLiteral("rres1/https:%2F%2Fs%2Fcal%2Fwork%2F"),
            QStringLiteral("rres1/with,comma"),
            QStringLiteral("rres2/"),
            QStringLiteral("c7"), // pathless fallback entries live alongside
        };

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("testrc"));
        {
            KConfig config(path);
            KConfigGroup group = config.group(QStringLiteral("GlobalCollectionSelection"));
            group.writeEntry("Selection", keys);
            config.sync();
        }

        KConfig reread(path);
        const KConfigGroup group = reread.group(QStringLiteral("GlobalCollectionSelection"));
        QCOMPARE(group.readEntry("Selection", QStringList()), keys);
    }

    void shouldWriteIdKeyByDefault()
    {
        // Default format is unchanged, so other consumers keep the legacy behavior.
        QStandardItemModel model;
        auto *root = collectionItem(1, QStringLiteral("res1"), QStringLiteral("res1root"));
        auto *calendar = collectionItem(2, QStringLiteral("res1"), QStringLiteral("work"));
        root->appendRow(calendar);
        model.appendRow(root);

        TestStateSaver saver;
        QCOMPARE(saver.indexToConfigString(calendar->index()), QStringLiteral("c2"));
    }
};

QTEST_MAIN(ETMViewStateSaverTest)

#include "etmviewstatesavertest.moc"
