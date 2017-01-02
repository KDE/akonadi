/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_CACHECLEANER_H
#define AKONADI_CACHECLEANER_H

#include "collectionscheduler.h"

#include <QMutex>

namespace Akonadi {
namespace Server {

class Collection;

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
    explicit CacheCleanerInhibitor(bool inhibit = true);
    ~CacheCleanerInhibitor();

    void inhibit();
    void uninhibit();

private:
    Q_DISABLE_COPY(CacheCleanerInhibitor)
    static QMutex sLock;
    static int sInhibitCount;
    bool mInhibited;
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
    ~CacheCleaner();

protected:
    void collectionExpired(const Collection &collection) Q_DECL_OVERRIDE;
    int collectionScheduleInterval(const Collection &collection) Q_DECL_OVERRIDE;
    bool hasChanged(const Collection &collection, const Collection &changed) Q_DECL_OVERRIDE;
    bool shouldScheduleCollection(const Collection &collection) Q_DECL_OVERRIDE;

private:
    static CacheCleaner *sInstance;

    friend class CacheCleanerInhibitor;

};

} // namespace Server
} // namespace Akonadi

#endif
