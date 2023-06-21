/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cachecleaner.h"
#include "akonadi.h"
#include "akonadiserver_debug.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/parthelper.h"
#include "storage/selectquerybuilder.h"

#include <private/protocol_p.h>

#include <QDateTime>

using namespace Akonadi::Server;

QMutex CacheCleanerInhibitor::sLock;
int CacheCleanerInhibitor::sInhibitCount = 0;

CacheCleanerInhibitor::CacheCleanerInhibitor(AkonadiServer &akonadi, bool doInhibit)
    : mCleaner(akonadi.cacheCleaner())
{
    if (doInhibit) {
        inhibit();
    }
}

CacheCleanerInhibitor::~CacheCleanerInhibitor()
{
    if (mInhibited) {
        uninhibit();
    }
}

void CacheCleanerInhibitor::inhibit()
{
    if (mInhibited) {
        qCCritical(AKONADISERVER_LOG) << "Cannot recursively inhibit an inhibitor";
        return;
    }

    sLock.lock();
    if (++sInhibitCount == 1) {
        if (mCleaner) {
            mCleaner->inhibit(true);
        }
    }
    sLock.unlock();
    mInhibited = true;
}

void CacheCleanerInhibitor::uninhibit()
{
    if (!mInhibited) {
        qCCritical(AKONADISERVER_LOG) << "Cannot uninhibit an uninhibited inhibitor"; // aaaw yeah
        return;
    }
    mInhibited = false;

    sLock.lock();
    Q_ASSERT(sInhibitCount > 0);
    if (--sInhibitCount == 0) {
        if (mCleaner) {
            mCleaner->inhibit(false);
        }
    }
    sLock.unlock();
}

CacheCleaner::CacheCleaner(QObject *parent)
    : CollectionScheduler(QStringLiteral("CacheCleaner"), QThread::IdlePriority, parent)
{
    setMinimumInterval(5);
}

CacheCleaner::~CacheCleaner()
{
    quitThread();
}

int CacheCleaner::collectionScheduleInterval(const Collection &collection)
{
    return collection.cachePolicyCacheTimeout();
}

bool CacheCleaner::hasChanged(const Collection &collection, const Collection &changed)
{
    return collection.cachePolicyLocalParts() != changed.cachePolicyLocalParts() || collection.cachePolicyCacheTimeout() != changed.cachePolicyCacheTimeout()
        || collection.cachePolicyInherit() != changed.cachePolicyInherit();
}

bool CacheCleaner::shouldScheduleCollection(const Collection &collection)
{
    return collection.cachePolicyLocalParts() != QLatin1String("ALL") && collection.cachePolicyCacheTimeout() >= 0
        && (collection.enabled() || (collection.displayPref() == Collection::True) || (collection.syncPref() == Collection::True)
            || (collection.indexPref() == Collection::True))
        && collection.resourceId() > 0;
}

void CacheCleaner::collectionExpired(const Collection &collection)
{
    SelectQueryBuilder<Part> qb;
    qb.addJoin(QueryBuilder::InnerJoin, PimItem::tableName(), Part::pimItemIdColumn(), PimItem::idFullColumnName());
    qb.addJoin(QueryBuilder::InnerJoin, PartType::tableName(), Part::partTypeIdFullColumnName(), PartType::idFullColumnName());
    qb.addValueCondition(PimItem::collectionIdFullColumnName(), Query::Equals, collection.id());
    qb.addValueCondition(PimItem::atimeFullColumnName(), Query::Less, QDateTime::currentDateTimeUtc().addSecs(-60 * collection.cachePolicyCacheTimeout()));
    qb.addValueCondition(Part::dataFullColumnName(), Query::IsNot, QVariant());
    qb.addValueCondition(PartType::nsFullColumnName(), Query::Equals, QLatin1String("PLD"));
    qb.addValueCondition(PimItem::dirtyFullColumnName(), Query::Equals, false);

    const QStringList partNames = collection.cachePolicyLocalParts().split(QLatin1Char(' '));
    for (QString partName : partNames) {
        if (partName.startsWith(QLatin1String(AKONADI_PARAM_PLD))) {
            partName.remove(0, 4);
        }
        qb.addValueCondition(PartType::nameFullColumnName(), Query::NotEquals, partName);
    }
    if (qb.exec()) {
        const Part::List parts = qb.result();
        if (!parts.isEmpty()) {
            qCInfo(AKONADISERVER_LOG) << "CacheCleaner found" << parts.count() << "item parts to expire in collection" << collection.name();
            // clear data field
            for (Part part : parts) {
                try {
                    if (!PartHelper::truncate(part)) {
                        qCWarning(AKONADISERVER_LOG) << "CacheCleaner failed to expire item part" << part.id();
                    }
                } catch (const PartHelperException &e) {
                    qCCritical(AKONADISERVER_LOG) << e.type() << e.what();
                }
            }
        }
    }
}

#include "moc_cachecleaner.cpp"
