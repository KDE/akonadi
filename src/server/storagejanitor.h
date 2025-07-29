/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akthread.h"
#include "storage/dbconfig.h"

#include <QDBusConnection>

namespace Akonadi
{
namespace Server
{
class Collection;
class AkonadiServer;
class DataStore;

/**
 * Various database checking/maintenance features.
 */
class StorageJanitor : public AkThread
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.Janitor")

protected:
    /**
     * Use AkThread::create() to create and start a new StorageJanitor thread.
     */
    explicit StorageJanitor(AkonadiServer *mAkonadi, DbConfig *config = DbConfig::configuredDatabase());

public:
    /**
     * Use AkThread::create() to run StorageJanitor in a separate thread. Only use this constructor
     * if you want to run StorageJanitor in the current thread.
     */
    explicit StorageJanitor(DbConfig *config);
    ~StorageJanitor() override;

public Q_SLOTS:
    /** Triggers a consistency check of the internal storage. */
    Q_SCRIPTABLE Q_NOREPLY void check();
    /** Triggers a vacuuming of the database, that is compacting of unused space. */
    Q_SCRIPTABLE Q_NOREPLY void vacuum();

Q_SIGNALS:
    /** Sends informational messages to a possible UI for this. */
    Q_SCRIPTABLE void information(const QString &msg);
    Q_SCRIPTABLE void done();

protected:
    void init() override;
    void quit() override;

private:
    void registerTasks();

    void inform(const char *msg);
    void inform(const QString &msg);
    /** Create a lost+found collection if necessary. */
    qint64 lostAndFoundCollection();

    /**
     * Look for resources in the DB not existing in reality.
     */
    void findOrphanedResources();

    /**
     * Look for collections belonging to non-existent resources.
     */
    void findOrphanedCollections();

    /**
     * Verifies that each collection in the collection tree has a path to the root
     * and that all collections along that path belong to the same resource.
     */
    void checkCollectionTreeConsistency();
    void checkPathToRoot(const Collection &col);

    /**
     * Look for items belonging to non-existing collections.
     */
    void findOrphanedItems();

    /**
     * Look for parts belonging to non-existing items.
     */
    void findOrphanedParts();

    /**
     * Look for item flags belonging to non-existing items.
     */
    void findOrphanedPimItemFlags();

    /**
     * Look for duplicate item flags and fix them.
     */
    void findDuplicateFlags();

    /**
     * Look for duplicate mime type names and fix them.
     */
    void findDuplicateMimeTypes();

    /**
     * Look for duplicate part type names and fix them.
     */
    void findDuplicatePartTypes();

    /**
     * Look for duplicate tag names and fix them.
     */
    void findDuplicateTagTypes();

    /**
     * Look for parts referring to the same external file.
     */
    void findOverlappingParts();

    /**
     * Verify fs and db part state.
     */
    void verifyExternalParts();

    /**
     * Look for dirty objects.
     */
    void findDirtyObjects();

    /**
     * Look for duplicates by RID.
     *
     * ..and remove the one that doesn't match the parent collections content mimetype.
     */
    void findRIDDuplicates();

    /**
     * Check whether part sizes match what's in database.
     *
     * If SizeTreshold has change, it will move parts from or to database
     * where necessary.
     */
    void checkSizeTreshold();

    /**
     * Check if all external payload files are migrated to the levelled folder
     * hierarchy and migrates them if necessary
     */
    void migrateToLevelledCacheHierarchy();

    /**
     * Check if the search index contains any entries that refer to Akonadi
     * Items that no longer exist in the DB.
     */
    void findOrphanSearchIndexEntries();

    /**
     * Make sure that the "Search" collection in the virtual search resource
     * exists. It is only created during database initialization, so if user
     * somehow manages to delete it, their search would be completely borked.
     */
    void ensureSearchCollection();

    /**
     * Clear cache that holds pre-computed collection statistics.
     * They will be re-computed on demand.
     */
    void expireCollectionStatisticsCache();

private:
    qint64 m_lostFoundCollectionId;
    AkonadiServer *m_akonadi = nullptr;
    DbConfig *m_dbConfig = nullptr;
    std::unique_ptr<DataStore> m_dataStore;

    struct Task {
        QString name;
        void (StorageJanitor::*func)();
    };
    QList<Task> m_tasks;
};

} // namespace Server
} // namespace Akonadi
