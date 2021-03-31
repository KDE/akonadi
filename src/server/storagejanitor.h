/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akthread.h"

#include <QDBusConnection>

namespace Akonadi
{
namespace Server
{
class Collection;
class AkonadiServer;

/**
 * Various database checking/maintenance features.
 */
class StorageJanitor : public AkThread
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.Janitor")

public:
    explicit StorageJanitor(AkonadiServer &mAkonadi);
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
     * Verifies there is a path from @p col to the root of the collection tree
     * and that everything along that path belongs to the same resource.
     */
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

private:
    qint64 m_lostFoundCollectionId;
    AkonadiServer &m_akonadi;
};

} // namespace Server
} // namespace Akonadi

