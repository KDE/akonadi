/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "specialcollections.h"
#include "akonadicore_debug.h"
#include "specialcollections_p.h"
#include "specialcollectionattribute.h"

#include "agentinstance.h"
#include "agentmanager.h"
#include "monitor.h"
#include "collectionfetchscope.h"
#include "interface.h"

#include <KCoreConfigSkeleton>

#include <QHash>

using namespace Akonadi;

SpecialCollectionsPrivate::SpecialCollectionsPrivate(KCoreConfigSkeleton *settings, SpecialCollections *qq)
    : q(qq)
    , mSettings(settings)
    , mBatchMode(false)
{
    mMonitor = new Monitor(q);
    mMonitor->setObjectName(QStringLiteral("SpecialCollectionsMonitor"));
    mMonitor->fetchCollectionStatistics(true);

    /// In order to know if items are added or deleted
    /// from one of our specialcollection folders,
    /// we have to watch all mail item add/move/delete notifications
    /// and check for the parent to see if it is one we care about
    QObject::connect(mMonitor, &Monitor::collectionRemoved,
                     q, [this](const Akonadi::Collection &col) { collectionRemoved(col); });
    QObject::connect(mMonitor, &Monitor::collectionStatisticsChanged,
                     q, [this](Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &statistics)
    { collectionStatisticsChanged(id, statistics); });
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
        qCDebug(AKONADICORE_LOG) << "Emitting changed for" << resourceId;
        const AgentInstance agentInstance = AgentManager::self()->instance(resourceId);
        Q_EMIT q->collectionsChanged(agentInstance);
        // first compare with local value then with config value (which also updates the local value)
        if (resourceId == mDefaultResourceId || resourceId == defaultResourceId()) {
            qCDebug(AKONADICORE_LOG) << "Emitting defaultFoldersChanged.";
            Q_EMIT q->defaultCollectionsChanged();
        }
    }
}

void SpecialCollectionsPrivate::collectionRemoved(const Collection &collection)
{
    qCDebug(AKONADICORE_LOG) << "Collection" << collection.id() << "resource" << collection.resource();
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
    CollectionFetchOptions options;
    options.fetchScope().setAncestorRetrieval(CollectionFetchScope::None);
    Akonadi::fetchCollection(Collection{collectionId}, options).then(
        [this, statistics](const Collection &collection) {
            mFoldersForResource[collection.resource()][collection.name().toUtf8()].setStatistics(statistics);
        },
        [](const Error &error) {
            qCWarning(AKONADICORE_LOG) << "Error fetching collection to get name from id for statistics updating in specialCollections:" << error;
        });
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

    for (const QString &resourceId : qAsConst(mToEmitChangedFor)) {
        emitChanged(resourceId);
    }

    mToEmitChangedFor.clear();
}

void SpecialCollectionsPrivate::forgetFoldersForResource(const QString &resourceId)
{
    if (mFoldersForResource.contains(resourceId)) {
        const auto folders = mFoldersForResource[resourceId];
        for (const auto &collection : folders) {
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
        auto *attribute = attributeCollection.attribute<SpecialCollectionAttribute>(Collection::AddIfMissing);
        attribute->setCollectionType(type);
        Akonadi::updateCollection(attributeCollection);
    }
}

void SpecialCollections::unsetSpecialCollection(const Akonadi::Collection &collection)
{
    if (collection.hasAttribute<SpecialCollectionAttribute>()) {
        Collection attributeCollection(collection);
        attributeCollection.removeAttribute<SpecialCollectionAttribute>();
        Akonadi::updateCollection(attributeCollection);
    }
}

bool SpecialCollections::unregisterCollection(const Collection &collection)
{
    if (!collection.isValid()) {
        qCWarning(AKONADICORE_LOG) << "Invalid collection.";
        return false;
    }

    const QString &resourceId = collection.resource();
    if (resourceId.isEmpty()) {
        qCWarning(AKONADICORE_LOG) << "Collection has empty resourceId.";
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
        qCWarning(AKONADICORE_LOG) << "Invalid collection.";
        return false;
    }

    const QString &resourceId = collection.resource();
    if (resourceId.isEmpty()) {
        qCWarning(AKONADICORE_LOG) << "Collection has empty resourceId.";
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
