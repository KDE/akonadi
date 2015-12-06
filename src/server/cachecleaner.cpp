/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (C) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "cachecleaner.h"
#include "storage/parthelper.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/entity.h"
#include "akonadi.h"

#include <shared/akdebug.h>
#include <private/protocol_p.h>

using namespace Akonadi::Server;

QMutex CacheCleanerInhibitor::sLock;
int CacheCleanerInhibitor::sInhibitCount = 0;

CacheCleanerInhibitor::CacheCleanerInhibitor(bool doInhibit)
    : mInhibited(false)
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
        akError() << "Cannot recursively inhibit an inhibitor";
        return;
    }

    sLock.lock();
    if (++sInhibitCount == 1) {
        if (AkonadiServer::instance()->cacheCleaner()) {
            AkonadiServer::instance()->cacheCleaner()->inhibit(true);
        }
    }
    sLock.unlock();
    mInhibited = true;
}

void CacheCleanerInhibitor::uninhibit()
{
    if (!mInhibited) {
        akError() << "Cannot uninhibit an uninhibited inhibitor"; // aaaw yeah
        return;
    }
    mInhibited = false;

    sLock.lock();
    Q_ASSERT(sInhibitCount > 0);
    if (--sInhibitCount == 0) {
        if (AkonadiServer::instance()->cacheCleaner()) {
            AkonadiServer::instance()->cacheCleaner()->inhibit(false);
        }
    }
    sLock.unlock();
}

CacheCleaner::CacheCleaner(QObject *parent)
    : CollectionScheduler(QThread::IdlePriority, parent)
{
    setObjectName(QStringLiteral("CacheCleaner"));
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
    return collection.cachePolicyLocalParts() != changed.cachePolicyLocalParts()
           || collection.cachePolicyCacheTimeout() != changed.cachePolicyCacheTimeout()
           || collection.cachePolicyInherit() != changed.cachePolicyInherit();
}

bool CacheCleaner::shouldScheduleCollection(const Collection &collection)
{
    return collection.cachePolicyLocalParts() != QLatin1String("ALL")
           && collection.cachePolicyCacheTimeout() >= 0
           && (collection.enabled() || (collection.displayPref() == Tristate::True) || (collection.syncPref() == Tristate::True) || (collection.indexPref() == Tristate::True))
           && collection.resourceId() > 0;
}

void CacheCleaner::collectionExpired(const Collection &collection)
{
    SelectQueryBuilder<Part> qb;
    qb.addJoin(QueryBuilder::InnerJoin, PimItem::tableName(), Part::pimItemIdColumn(), PimItem::idFullColumnName());
    qb.addJoin(QueryBuilder::InnerJoin, PartType::tableName(), Part::partTypeIdFullColumnName(), PartType::idFullColumnName());
    qb.addValueCondition(PimItem::collectionIdFullColumnName(), Query::Equals, collection.id());
    qb.addValueCondition(PimItem::atimeFullColumnName(), Query::Less, QDateTime::currentDateTime().addSecs(-60 * collection.cachePolicyCacheTimeout()));
    qb.addValueCondition(Part::dataFullColumnName(), Query::IsNot, QVariant());
    qb.addValueCondition(PartType::nsFullColumnName(), Query::Equals, QLatin1String("PLD"));
    qb.addValueCondition(PimItem::dirtyFullColumnName(), Query::Equals, false);

    QStringList localParts;
    QStringList partNames = collection.cachePolicyLocalParts().split(QStringLiteral(" "));
    Q_FOREACH (QString partName, partNames) {
        if (partName.startsWith(QLatin1String(AKONADI_PARAM_PLD))) {
            partName = partName.mid(4);
        }
        qb.addValueCondition(PartType::nameFullColumnName(), Query::NotEquals, partName);
    }
    if (qb.exec()) {
        const Part::List parts = qb.result();
        if (!parts.isEmpty()) {
            akDebug() << "found" << parts.count() << "item parts to expire in collection" << collection.name();
            // clear data field
            Q_FOREACH (Part part, parts) {
                try {
                    if (!PartHelper::truncate(part)) {
                        akDebug() << "failed to update item part" << part.id();
                    }
                } catch (const PartHelperException &e) {
                    akError() << e.type() << e.what();
                }
            }
        }
    }
}
