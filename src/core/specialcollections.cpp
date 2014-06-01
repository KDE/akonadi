/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "specialcollections.h"

#include "specialcollections_p.h"
#include "specialcollectionattribute.h"

#include "agentinstance.h"
#include "agentmanager.h"
#include "collectionmodifyjob.h"
#include "collectionfetchjob.h"
#include "monitor.h"
#include "collectionfetchscope.h"

#include <QDebug>
#include <kcoreconfigskeleton.h>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSet>

using namespace Akonadi;

SpecialCollectionsPrivate::SpecialCollectionsPrivate(KCoreConfigSkeleton *settings, SpecialCollections *qq)
    : q(qq)
    , mSettings(settings)
    , mBatchMode(false)
{
    mMonitor = new Monitor(q);
    mMonitor->fetchCollectionStatistics(true);

    /// In order to know if items are added or deleted
    /// from one of our specialcollection folders,
    /// we have to watch all mail item add/move/delete notifications
    /// and check for the parent to see if it is one we care about
    QObject::connect(mMonitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
                     q, SLOT(collectionRemoved(Akonadi::Collection)));
    QObject::connect(mMonitor, SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)),
                     q, SLOT(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)));
}

SpecialCollectionsPrivate::~SpecialCollectionsPrivate()
{
}

QString SpecialCollectionsPrivate::defaultResourceId() const
{
    if (mDefaultResourceId.isEmpty()) {
        mSettings->load();
        const KConfigSkeletonItem *item = mSettings->findItem(QStringLiteral("DefaultResourceId"));
        Q_ASSERT(item);

        mDefaultResourceId = item->property().toString();
    }
    return mDefaultResourceId;
}

void SpecialCollectionsPrivate::emitChanged(const QString &resourceId)
{
    if (mBatchMode) {
        mToEmitChangedFor.insert(resourceId);
    } else {
        qDebug() << "Emitting changed for" << resourceId;
        const AgentInstance agentInstance = AgentManager::self()->instance(resourceId);
        emit q->collectionsChanged(agentInstance);
        // first compare with local value then with config value (which also updates the local value)
        if (resourceId == mDefaultResourceId || resourceId == defaultResourceId()) {
            qDebug() << "Emitting defaultFoldersChanged.";
            emit q->defaultCollectionsChanged();
        }
    }
}

void SpecialCollectionsPrivate::collectionRemoved(const Collection &collection)
{
    qDebug() << "Collection" << collection.id() << "resource" << collection.resource();
    if (mFoldersForResource.contains(collection.resource())) {

        // Retrieve the list of special folders for the resource the collection belongs to
        QHash<QByteArray, Collection> &folders = mFoldersForResource[collection.resource()];
        {
            QMutableHashIterator<QByteArray, Collection> it(folders);
            while (it.hasNext()) {
                it.next();
                if (it.value() == collection) {
                    // The collection to be removed is a special folder
                    it.remove();
                    emitChanged(collection.resource());
                }
            }
        }

        if (folders.isEmpty()) {
            // This resource has no more folders, so remove it completely.
            mFoldersForResource.remove(collection.resource());
        }
    }
}

void SpecialCollectionsPrivate::collectionStatisticsChanged(Akonadi::Collection::Id collectionId, const Akonadi::CollectionStatistics &statistics)
{
    // need to get the name of the collection in order to be able to check if we are storing it,
    // but we have the id from the monitor, so fetch the name.
    Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(Collection(collectionId), Akonadi::CollectionFetchJob::Base);
    fetchJob->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::None);
    fetchJob->setProperty("statistics", QVariant::fromValue(statistics));

    q->connect(fetchJob, SIGNAL(result(KJob*)), q, SLOT(collectionFetchJobFinished(KJob*)));
}

void SpecialCollectionsPrivate::collectionFetchJobFinished(KJob *job)
{
    if (job->error()) {
        qWarning() << "Error fetching collection to get name from id for statistics updating in specialcollections!";
        return;
    }

    const Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);

    Q_ASSERT(fetchJob->collections().size() > 0);
    const Akonadi::Collection collection = fetchJob->collections().first();
    const Akonadi::CollectionStatistics statistics = fetchJob->property("statistics").value<Akonadi::CollectionStatistics>();

    mFoldersForResource[collection.resource()][collection.name().toUtf8()].setStatistics(statistics);
}

void SpecialCollectionsPrivate::beginBatchRegister()
{
    Q_ASSERT(!mBatchMode);
    mBatchMode = true;
    Q_ASSERT(mToEmitChangedFor.isEmpty());
}

void SpecialCollectionsPrivate::endBatchRegister()
{
    Q_ASSERT(mBatchMode);
    mBatchMode = false;

    foreach (const QString &resourceId, mToEmitChangedFor) {
        emitChanged(resourceId);
    }

    mToEmitChangedFor.clear();
}

void SpecialCollectionsPrivate::forgetFoldersForResource(const QString &resourceId)
{
    if (mFoldersForResource.contains(resourceId)) {
        const Collection::List folders = mFoldersForResource[resourceId].values();

        foreach (const Collection &collection, folders) {
            mMonitor->setCollectionMonitored(collection, false);
        }

        mFoldersForResource.remove(resourceId);
        emitChanged(resourceId);
    }
}

AgentInstance SpecialCollectionsPrivate::defaultResource() const
{
    const QString identifier = defaultResourceId();
    return AgentManager::self()->instance(identifier);
}

SpecialCollections::SpecialCollections(KCoreConfigSkeleton *settings, QObject *parent)
    : QObject(parent)
    , d(new SpecialCollectionsPrivate(settings, this))
{
}

SpecialCollections::~SpecialCollections()
{
    delete d;
}

bool SpecialCollections::hasCollection(const QByteArray &type, const AgentInstance &instance) const
{
    return d->mFoldersForResource.value(instance.identifier()).contains(type);
}

Akonadi::Collection SpecialCollections::collection(const QByteArray &type, const AgentInstance &instance) const
{
    return d->mFoldersForResource.value(instance.identifier()).value(type);
}

void SpecialCollections::setSpecialCollectionType(const QByteArray &type, const Akonadi::Collection &collection)
{
    if (!collection.hasAttribute<SpecialCollectionAttribute>() || collection.attribute<SpecialCollectionAttribute>()->collectionType() != type) {
        Collection attributeCollection(collection);
        SpecialCollectionAttribute *attribute = attributeCollection.attribute<SpecialCollectionAttribute>(Collection::AddIfMissing);
        attribute->setCollectionType(type);
        new CollectionModifyJob(attributeCollection);
    }
}

void SpecialCollections::unsetSpecialCollection(const Akonadi::Collection &collection)
{
    if (collection.hasAttribute<SpecialCollectionAttribute>()) {
        Collection attributeCollection(collection);
        attributeCollection.removeAttribute<SpecialCollectionAttribute>();
        new CollectionModifyJob(attributeCollection);
    }
}

bool SpecialCollections::unregisterCollection(const Collection &collection)
{
    if (!collection.isValid()) {
        qWarning() << "Invalid collection.";
        return false;
    }

    const QString &resourceId = collection.resource();
    if (resourceId.isEmpty()) {
        qWarning() << "Collection has empty resourceId.";
        return false;
    }

    unsetSpecialCollection(collection);

    d->mMonitor->setCollectionMonitored(collection, false);
    //Remove from list of collection
    d->collectionRemoved(collection);
    return true;
}

bool SpecialCollections::registerCollection(const QByteArray &type, const Collection &collection)
{
    if (!collection.isValid()) {
        qWarning() << "Invalid collection.";
        return false;
    }

    const QString &resourceId = collection.resource();
    if (resourceId.isEmpty()) {
        qWarning() << "Collection has empty resourceId.";
        return false;
    }

    setSpecialCollectionType(type, collection);

    const Collection oldCollection = d->mFoldersForResource.value(resourceId).value(type);
    if (oldCollection != collection) {
        if (oldCollection.isValid()) {
            d->mMonitor->setCollectionMonitored(oldCollection, false);
        }
        d->mMonitor->setCollectionMonitored(collection, true);
        d->mFoldersForResource[resourceId].insert(type, collection);
        d->emitChanged(resourceId);
    }

    return true;
}

bool SpecialCollections::hasDefaultCollection(const QByteArray &type) const
{
    return hasCollection(type, d->defaultResource());
}

Akonadi::Collection SpecialCollections::defaultCollection(const QByteArray &type) const
{
    return collection(type, d->defaultResource());
}

#include "moc_specialcollections.cpp"
