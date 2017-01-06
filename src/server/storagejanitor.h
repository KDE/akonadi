/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#ifndef STORAGEJANITOR_H
#define STORAGEJANITOR_H

#include "akthread.h"

#include <QDBusConnection>

namespace Akonadi
{
namespace Server
{

class Collection;

/**
 * Various database checking/maintenance features.
 */
class StorageJanitor : public AkThread
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.Janitor")

public:
    explicit StorageJanitor(QObject *parent = nullptr);
    ~StorageJanitor();

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
    void init() Q_DECL_OVERRIDE;
    void quit() Q_DECL_OVERRIDE;

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
     * and that that everything along that path belongs to the same resource.
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

private:
    qint64 m_lostFoundCollectionId;
};

} // namespace Server
} // namespace Akonadi

#endif // STORAGEJANITOR_H
