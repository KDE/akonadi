/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "collectionreferencemanager.h"
#include "akonadi.h"
#include "akonadiserver_debug.h"
#include "cachecleaner.h"
#include "storage/selectquerybuilder.h"

using namespace Akonadi;
using namespace Akonadi::Server;

CollectionReferenceManager *CollectionReferenceManager::s_instance = 0;

CollectionReferenceManager::CollectionReferenceManager()
    : mReferenceLock(QMutex::Recursive)
{
}

CollectionReferenceManager *CollectionReferenceManager::instance()
{
    static QMutex s_instanceLock;
    QMutexLocker locker(&s_instanceLock);
    if (!s_instance) {
        s_instance = new CollectionReferenceManager();
    }
    return s_instance;
}

void CollectionReferenceManager::referenceCollection(const QByteArray &sessionId, const Collection &collection, bool reference)
{
    QMutexLocker locker(&mReferenceLock);
    if (reference) {
        if (!mReferenceMap.contains(collection.id(), sessionId)) {
            mReferenceMap.insert(collection.id(), sessionId);
        }
    } else {
        mReferenceMap.remove(collection.id(), sessionId);
        expireCollectionIfNecessary(collection.id());
    }
}

void CollectionReferenceManager::removeSession(const QByteArray &sessionId)
{
    QMutexLocker locker(&mReferenceLock);
    Q_FOREACH (Collection::Id col, mReferenceMap.keys(sessionId)) {
        mReferenceMap.remove(col, sessionId);
        expireCollectionIfNecessary(col);
        if (!isReferenced(col)) {
            Collection collection = Collection::retrieveById(col);
            collection.setReferenced(false);
            collection.update();
        }
    }
}

bool CollectionReferenceManager::isReferenced(Collection::Id collection) const
{
    QMutexLocker locker(&mReferenceLock);
    return mReferenceMap.contains(collection);
}

bool CollectionReferenceManager::isReferenced(Collection::Id collection, const QByteArray &sessionId) const
{
    QMutexLocker locker(&mReferenceLock);
    return mReferenceMap.contains(collection, sessionId);
}

void CollectionReferenceManager::expireCollectionIfNecessary(Collection::Id collection)
{
    QMutexLocker locker(&mReferenceLock);
    if (!isReferenced(collection)) {
        if (AkonadiServer::instance()->cacheCleaner()) {
            AkonadiServer::instance()->cacheCleaner()->collectionChanged(collection);
        }
    }
}

void CollectionReferenceManager::cleanup()
{
    SelectQueryBuilder<Collection> qb;
    qb.addValueCondition(Collection::referencedColumn(), Query::Equals, true);
    if (!qb.exec()) {
        qCCritical(AKONADISERVER_LOG) << "Failed to execute  collection reference cleanup query.";
        return;
    }
    Q_FOREACH (Collection col, qb.result()) {
        col.setReferenced(false);
        col.update();
        if (AkonadiServer::instance()->cacheCleaner()) {
            AkonadiServer::instance()->cacheCleaner()->collectionChanged(col.id());
        }
    }
    QMutexLocker locker(&instance()->mReferenceLock);
    instance()->mReferenceMap.clear();
}
