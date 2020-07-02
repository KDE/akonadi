/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_CACHECLEANER_H
#define AKONADI_CACHECLEANER_H

#include "collectionscheduler.h"

#include <QMutex>

namespace Akonadi
{
namespace Server
{

class Collection;
class CacheCleaner;
class AkonadiServer;

/**
 * A RAII helper class to temporarily stop the CacheCleaner. This allows long-lasting
 * operations to safely retrieve all data from resource and perform an operation on them
 * (like move or copy) without risking that the cache will be cleaned in the meanwhile
 *
 * The inhibitor is recursive, so it's possible to create multiple instances of the
 * CacheCleanerInhibitor and the CacheCleaner will be inhibited until all instances
 * are destroyed again. However it's not possible to inhibit a single inhibitor
 * multiple times.
 */
class CacheCleanerInhibitor
{
public:
    explicit CacheCleanerInhibitor(AkonadiServer &akonadi, bool inhibit = true);
    ~CacheCleanerInhibitor();

    void inhibit();
    void uninhibit();

private:
    Q_DISABLE_COPY(CacheCleanerInhibitor)
    static QMutex sLock;
    static int sInhibitCount;

    CacheCleaner *mCleaner = nullptr;
    bool mInhibited = false;
};

/**
  Cache cleaner.
 */
class CacheCleaner : public CollectionScheduler
{
    Q_OBJECT

public:
    /**
      Creates a new cache cleaner thread.
      @param parent The parent object.
    */
    explicit CacheCleaner(QObject *parent = nullptr);
    ~CacheCleaner() override;

protected:
    void collectionExpired(const Collection &collection) override;
    int collectionScheduleInterval(const Collection &collection) override;
    bool hasChanged(const Collection &collection, const Collection &changed) override;
    bool shouldScheduleCollection(const Collection &collection) override;

private:
    friend class CacheCleanerInhibitor;

};

} // namespace Server
} // namespace Akonadi

#endif
