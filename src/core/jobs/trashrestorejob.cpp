/*
 *    SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "trashrestorejob.h"

#include "entitydeletedattribute.h"
#include "job_p.h"

#include "trashsettings.h"

#include <KLocalizedString>

#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "collectionmodifyjob.h"
#include "collectionmovejob.h"
#include "itemdeletejob.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"

#include "akonadicore_debug.h"

#include <QHash>

using namespace Akonadi;

class Akonadi::TrashRestoreJobPrivate : public JobPrivate
{
public:
    explicit TrashRestoreJobPrivate(TrashRestoreJob *parent)
        : JobPrivate(parent)
    {
    }

    void selectResult(KJob *job);

    // Called when the target collection was fetched,
    // will issue the move and the removal of the attributes if collection is valid
    void targetCollectionFetched(KJob *job);

    void removeAttribute(const Akonadi::Item::List &list);
    void removeAttribute(const Akonadi::Collection::List &list);

    // Called after initial fetch of items, issues fetch of target collection or removes attributes for in place restore
    void itemsReceived(const Akonadi::Item::List &items);
    void collectionsReceived(const Akonadi::Collection::List &collections);

    Q_DECLARE_PUBLIC(TrashRestoreJob)

    Item::List mItems;
    Collection mCollection;
    Collection mTargetCollection;
    QHash<Collection, Item::List> restoreCollections; // groups items to target restore collections
};

void TrashRestoreJobPrivate::selectResult(KJob *job)
{
    Q_Q(TrashRestoreJob);
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorString();
        return; // KCompositeJob takes care of errors
    }

    if (!q->hasSubjobs() || (q->subjobs().contains(static_cast<KJob *>(q->sender())) && q->subjobs().size() == 1)) {
        // qCWarning(AKONADICORE_LOG) << "trash restore finished";
        q->emitResult();
    }
}

void TrashRestoreJobPrivate::targetCollectionFetched(KJob *job)
{
    Q_Q(TrashRestoreJob);

    auto fetchJob = qobject_cast<CollectionFetchJob *>(job);
    Q_ASSERT(fetchJob);
    const Collection::List &list = fetchJob->collections();

    if (list.isEmpty() || !list.first().isValid() || list.first().hasAttribute<Akonadi::EntityDeletedAttribute>()) { // target collection is invalid/not
                                                                                                                     // existing

        const QString res = fetchJob->property("Resource").toString();
        if (res.isEmpty()) { // There is no fallback
            q->setError(Job::Unknown);
            q->setErrorText(i18n("Could not find restore collection and restore resource is not available"));
            q->emitResult();
            // FAIL
            qCWarning(AKONADICORE_LOG) << "restore collection not available";
            return;
        }

        // Try again with the root collection of the resource as fallback
        auto resRootFetch = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel, q);
        resRootFetch->fetchScope().setResource(res);
        const QVariant &var = fetchJob->property("Items");
        if (var.isValid()) {
            resRootFetch->setProperty("Items", var.toInt());
        }
        q->connect(resRootFetch, &KJob::result, q, [this](KJob *job) {
            targetCollectionFetched(job);
        });
        q->connect(resRootFetch, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
        return;
    }
    Q_ASSERT(list.size() == 1);
    // SUCCESS
    // We know where to move the entity, so remove the attributes and move them to the right location
    if (!mItems.isEmpty()) {
        const QVariant &var = fetchJob->property("Items");
        Q_ASSERT(var.isValid());
        const Item::List &items = restoreCollections[Collection(var.toInt())];

        // store removed attribute if destination collection is valid or the item doesn't have a restore collection
        // TODO only remove the attribute if the move job was successful (although it is unlikely that it fails since we already fetched the collection)
        removeAttribute(items);
        if (items.first().parentCollection() != list.first()) {
            auto job = new ItemMoveJob(items, list.first(), q);
            q->connect(job, &KJob::result, q, [this](KJob *job) {
                selectResult(job);
            });
        }
    } else {
        Q_ASSERT(mCollection.isValid());
        // TODO only remove the attribute if the move job was successful
        removeAttribute(Collection::List() << mCollection);
        auto collectionFetchJob = new CollectionFetchJob(mCollection, CollectionFetchJob::Recursive, q);
        q->connect(collectionFetchJob, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
        q->connect(collectionFetchJob, &CollectionFetchJob::collectionsReceived, q, [this](const auto &cols) {
            removeAttribute(cols);
        });

        if (mCollection.parentCollection() != list.first()) {
            auto job = new CollectionMoveJob(mCollection, list.first(), q);
            q->connect(job, &KJob::result, q, [this](KJob *job) {
                selectResult(job);
            });
        }
    }
}

void TrashRestoreJobPrivate::itemsReceived(const Akonadi::Item::List &items)
{
    Q_Q(TrashRestoreJob);
    if (items.isEmpty()) {
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Invalid items passed"));
        q->emitResult();
        return;
    }
    mItems = items;

    // Sort by restore collection
    for (const Item &item : std::as_const(mItems)) {
        if (!item.hasAttribute<Akonadi::EntityDeletedAttribute>()) {
            continue;
        }
        // If the restore collection is invalid we restore the item in place, so we don't need to know its restore resource => we can put those cases in the
        // same list
        restoreCollections[item.attribute<Akonadi::EntityDeletedAttribute>()->restoreCollection()].append(item);
    }

    for (auto it = restoreCollections.cbegin(), e = restoreCollections.cend(); it != e; ++it) {
        const Item &first = it.value().first();
        // Move the items to the correct collection if available
        Collection targetCollection = it.key();
        const QString restoreResource = first.attribute<Akonadi::EntityDeletedAttribute>()->restoreResource();

        // Restore in place if no restore collection is set
        if (!targetCollection.isValid()) {
            removeAttribute(it.value());
            return;
        }

        // Explicit target overrides the resource
        if (mTargetCollection.isValid()) {
            targetCollection = mTargetCollection;
        }

        // Try to fetch the target resource to see if it is available
        auto fetchJob = new CollectionFetchJob(targetCollection, Akonadi::CollectionFetchJob::Base, q);
        if (!mTargetCollection.isValid()) { // explicit targets don't have a fallback
            fetchJob->setProperty("Resource", restoreResource);
        }
        fetchJob->setProperty("Items", it.key().id()); // to find the items in restore collections again
        q->connect(fetchJob, &KJob::result, q, [this](KJob *job) {
            targetCollectionFetched(job);
        });
    }
}

void TrashRestoreJobPrivate::collectionsReceived(const Akonadi::Collection::List &collections)
{
    Q_Q(TrashRestoreJob);
    if (collections.isEmpty()) {
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Invalid collection passed"));
        q->emitResult();
        return;
    }
    Q_ASSERT(collections.size() == 1);
    mCollection = collections.first();

    if (!mCollection.hasAttribute<Akonadi::EntityDeletedAttribute>()) {
        return;
    }

    const QString restoreResource = mCollection.attribute<Akonadi::EntityDeletedAttribute>()->restoreResource();
    Collection targetCollection = mCollection.attribute<EntityDeletedAttribute>()->restoreCollection();

    // Restore in place if no restore collection/resource is set
    if (!targetCollection.isValid()) {
        removeAttribute(Collection::List() << mCollection);
        auto collectionFetchJob = new CollectionFetchJob(mCollection, CollectionFetchJob::Recursive, q);
        q->connect(collectionFetchJob, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
        q->connect(collectionFetchJob, &CollectionFetchJob::collectionsReceived, q, [this](const auto &cols) {
            removeAttribute(cols);
        });
        return;
    }

    // Explicit target overrides the resource/configured restore collection
    if (mTargetCollection.isValid()) {
        targetCollection = mTargetCollection;
    }

    // Fetch the target collection to check if it's valid
    auto fetchJob = new CollectionFetchJob(targetCollection, CollectionFetchJob::Base, q);
    if (!mTargetCollection.isValid()) { // explicit targets don't have a fallback
        fetchJob->setProperty("Resource", restoreResource);
    }
    q->connect(fetchJob, &KJob::result, q, [this](KJob *job) {
        targetCollectionFetched(job);
    });
}

void TrashRestoreJobPrivate::removeAttribute(const Akonadi::Collection::List &list)
{
    Q_Q(TrashRestoreJob);
    QVectorIterator<Collection> i(list);
    while (i.hasNext()) {
        Collection col = i.next();
        col.removeAttribute<EntityDeletedAttribute>();

        auto job = new CollectionModifyJob(col, q);
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });

        auto itemFetchJob = new ItemFetchJob(col, q);
        itemFetchJob->fetchScope().fetchAttribute<EntityDeletedAttribute>(true);
        q->connect(itemFetchJob, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
        q->connect(itemFetchJob, &ItemFetchJob::itemsReceived, q, [this](const auto &items) {
            removeAttribute(items);
        });
    }
}

void TrashRestoreJobPrivate::removeAttribute(const Akonadi::Item::List &list)
{
    Q_Q(TrashRestoreJob);
    Item::List items = list;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QMutableVectorIterator<Item> i(items);
#else
    QMutableListIterator<Item> i(items);
#endif
    while (i.hasNext()) {
        Item &item = i.next();
        item.removeAttribute<EntityDeletedAttribute>();
        auto job = new ItemModifyJob(item, q);
        job->setIgnorePayload(true);
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
    }
    // For some reason it is not possible to apply this change to multiple items at once
    // ItemModifyJob *job = new ItemModifyJob(items, q);
    // q->connect( job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)) );
}

TrashRestoreJob::TrashRestoreJob(const Item &item, QObject *parent)
    : Job(new TrashRestoreJobPrivate(this), parent)
{
    Q_D(TrashRestoreJob);
    d->mItems << item;
}

TrashRestoreJob::TrashRestoreJob(const Item::List &items, QObject *parent)
    : Job(new TrashRestoreJobPrivate(this), parent)
{
    Q_D(TrashRestoreJob);
    d->mItems = items;
}

TrashRestoreJob::TrashRestoreJob(const Collection &collection, QObject *parent)
    : Job(new TrashRestoreJobPrivate(this), parent)
{
    Q_D(TrashRestoreJob);
    d->mCollection = collection;
}

TrashRestoreJob::~TrashRestoreJob()
{
}

void TrashRestoreJob::setTargetCollection(const Akonadi::Collection &collection)
{
    Q_D(TrashRestoreJob);
    d->mTargetCollection = collection;
}

Item::List TrashRestoreJob::items() const
{
    Q_D(const TrashRestoreJob);
    return d->mItems;
}

void TrashRestoreJob::doStart()
{
    Q_D(TrashRestoreJob);

    // We always have to fetch the entities to ensure that the EntityDeletedAttribute is available
    if (!d->mItems.isEmpty()) {
        auto job = new ItemFetchJob(d->mItems, this);
        job->fetchScope().setCacheOnly(true);
        job->fetchScope().fetchAttribute<EntityDeletedAttribute>(true);
        connect(job, &ItemFetchJob::itemsReceived, this, [d](const auto &items) {
            d->itemsReceived(items);
        });
    } else if (d->mCollection.isValid()) {
        auto job = new CollectionFetchJob(d->mCollection, CollectionFetchJob::Base, this);
        connect(job, &CollectionFetchJob::collectionsReceived, this, [d](const auto &cols) {
            d->collectionsReceived(cols);
        });
    } else {
        qCWarning(AKONADICORE_LOG) << "No valid collection or empty itemlist";
        setError(Job::Unknown);
        setErrorText(i18n("No valid collection or empty itemlist"));
        emitResult();
    }
}

#include "moc_trashrestorejob.cpp"
