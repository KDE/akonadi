/*
 *    Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>
 *
 *    This library is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU Library General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at your
 *    option) any later version.
 *
 *    This library is distributed in the hope that it will be useful, but WITHOUT
 *    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *    License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to the
 *    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *    02110-1301, USA.
 */

#include "trashrestorejob.h"

#include "collection.h"
#include "entitydeletedattribute.h"
#include "item.h"
#include "job_p.h"
#include "trashsettings.h"

#include <KLocalizedString>

#include "itemdeletejob.h"
#include "collectiondeletejob.h"
#include "itemmovejob.h"
#include "collectionmovejob.h"
#include "itemmodifyjob.h"
#include "collectionmodifyjob.h"
#include "collectionfetchjob.h"
#include "itemfetchjob.h"
#include "collectionfetchscope.h"
#include "itemfetchscope.h"

#include <QHash>

using namespace Akonadi;

class TrashRestoreJob::TrashRestoreJobPrivate : public JobPrivate
{
public:
    TrashRestoreJobPrivate(TrashRestoreJob *parent)
        : JobPrivate(parent)
    {
    }

    void selectResult(KJob *job);

    //Called when the target collection was fetched,
    //will issue the move and the removal of the attributes if collection is valid
    void targetCollectionFetched(KJob *job);

    void removeAttribute(const Akonadi::Item::List &list);
    void removeAttribute(const Akonadi::Collection::List &list);

    //Called after initial fetch of items, issues fetch of target collection or removes attributes for in place restore
    void itemsReceived(const Akonadi::Item::List &items);
    void collectionsReceived(const Akonadi::Collection::List &collections);

    Q_DECLARE_PUBLIC(TrashRestoreJob)

    Item::List mItems;
    Collection mCollection;
    Collection mTargetCollection;
    QHash<Collection, Item::List> restoreCollections; //groups items to target restore collections

};

void TrashRestoreJob::TrashRestoreJobPrivate::selectResult(KJob *job)
{
    Q_Q(TrashRestoreJob);
    if (job->error()) {
        qWarning() << job->errorString();
        return; // KCompositeJob takes care of errors
    }

    if (!q->hasSubjobs() || (q->subjobs().contains(static_cast<KJob *>(q->sender())) && q->subjobs().size() == 1)) {
        //qWarning() << "trash restore finished";
        q->emitResult();
    }
}

void TrashRestoreJob::TrashRestoreJobPrivate::targetCollectionFetched(KJob *job)
{
    Q_Q(TrashRestoreJob);

    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob *> (job);
    Q_ASSERT(fetchJob);
    const Collection::List &list = fetchJob->collections();

    if (list.isEmpty() || !list.first().isValid() || list.first().hasAttribute<Akonadi::EntityDeletedAttribute>()) {   //target collection is invalid/not existing

        const QString res = fetchJob->property("Resource").toString();
        if (res.isEmpty()) {   //There is no fallback
            q->setError(Job::Unknown);
            q->setErrorText(i18n("Could not find restore collection and restore resource is not available"));
            q->emitResult();
            //FAIL
            qWarning() << "restore collection not available";
            return;
        }

        //Try again with the root collection of the resource as fallback
        CollectionFetchJob *resRootFetch = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel, q);
        resRootFetch->fetchScope().setResource(res);
        const QVariant &var = fetchJob->property("Items");
        if (var.isValid()) {
            resRootFetch->setProperty("Items", var.toInt());
        }
        q->connect(resRootFetch, SIGNAL(result(KJob*)), SLOT(targetCollectionFetched(KJob*)));
        q->connect(resRootFetch, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
        return;
    }
    Q_ASSERT(list.size() == 1);
    //SUCCESS
    //We know where to move the entity, so remove the attributes and move them to the right location
    if (!mItems.isEmpty()) {
        const QVariant &var = fetchJob->property("Items");
        Q_ASSERT(var.isValid());
        const Item::List &items = restoreCollections[Collection(var.toInt())];

        //store removed attribute if destination collection is valid or the item doesn't have a restore collection
        //TODO only remove the attribute if the move job was successful (although it is unlikely that it fails since we already fetched the collection)
        removeAttribute(items);
        if (items.first().parentCollection() != list.first()) {
            ItemMoveJob *job = new ItemMoveJob(items, list.first(), q);
            q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
        }
    } else {
        Q_ASSERT(mCollection.isValid());
        //TODO only remove the attribute if the move job was successful
        removeAttribute(Collection::List() << mCollection);
        CollectionFetchJob *collectionFetchJob = new CollectionFetchJob(mCollection, CollectionFetchJob::Recursive, q);
        q->connect(collectionFetchJob, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
        q->connect(collectionFetchJob, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(removeAttribute(Akonadi::Collection::List)));

        if (mCollection.parentCollection() != list.first()) {
            CollectionMoveJob *job = new CollectionMoveJob(mCollection, list.first(), q);
            q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
        }
    }

}

void TrashRestoreJob::TrashRestoreJobPrivate::itemsReceived(const Akonadi::Item::List &items)
{
    Q_Q(TrashRestoreJob);
    if (items.isEmpty()) {
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Invalid items passed"));
        q->emitResult();
        return;
    }
    mItems = items;

    //Sort by restore collection
    foreach (const Item &item, mItems) {
        if (!item.hasAttribute<Akonadi::EntityDeletedAttribute>()) {
            continue;
        }
        //If the restore collection is invalid we restore the item in place, so we don't need to know its restore resource => we can put those cases in the same list
        restoreCollections[item.attribute<Akonadi::EntityDeletedAttribute>()->restoreCollection()].append(item);
    }

    foreach (const Collection &col, restoreCollections.keys()) {  //krazy:exclude=foreach
        const Item &first = restoreCollections.value(col).first();
        //Move the items to the correct collection if available
        Collection targetCollection = col;
        const QString restoreResource = first.attribute<Akonadi::EntityDeletedAttribute>()->restoreResource();

        //Restore in place if no restore collection is set
        if (!targetCollection.isValid()) {
            removeAttribute(restoreCollections.value(col));
            return;
        }

        //Explicit target Q_DECL_OVERRIDEs the resource
        if (mTargetCollection.isValid()) {
            targetCollection = mTargetCollection;
        }

        //Try to fetch the target resource to see if it is available
        CollectionFetchJob *fetchJob = new CollectionFetchJob(targetCollection, Akonadi::CollectionFetchJob::Base, q);
        if (!mTargetCollection.isValid()) {   //explicit targets don't have a fallback
            fetchJob->setProperty("Resource", restoreResource);
        }
        fetchJob->setProperty("Items", col.id());    //to find the items in restore collections again
        q->connect(fetchJob, SIGNAL(result(KJob*)), SLOT(targetCollectionFetched(KJob*)));
    }
}

void TrashRestoreJob::TrashRestoreJobPrivate::collectionsReceived(const Akonadi::Collection::List &collections)
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

    //Restore in place if no restore collection/resource is set
    if (!targetCollection.isValid()) {
        removeAttribute(Collection::List() << mCollection);
        CollectionFetchJob *collectionFetchJob = new CollectionFetchJob(mCollection, CollectionFetchJob::Recursive, q);
        q->connect(collectionFetchJob, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
        q->connect(collectionFetchJob, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(removeAttribute(Akonadi::Collection::List)));
        return;
    }

    //Explicit target Q_DECL_OVERRIDEs the resource/configured restore collection
    if (mTargetCollection.isValid()) {
        targetCollection = mTargetCollection;
    }

    //Fetch the target collection to check if it's valid
    CollectionFetchJob *fetchJob = new CollectionFetchJob(targetCollection, CollectionFetchJob::Base, q);
    if (!mTargetCollection.isValid()) {   //explicit targets don't have a fallback
        fetchJob->setProperty("Resource", restoreResource);
    }
    q->connect(fetchJob, SIGNAL(result(KJob*)), SLOT(targetCollectionFetched(KJob*)));
}

void TrashRestoreJob::TrashRestoreJobPrivate::removeAttribute(const Akonadi::Collection::List &list)
{
    Q_Q(TrashRestoreJob);
    QListIterator<Collection> i(list);
    while (i.hasNext()) {
        Collection col = i.next();
        col.removeAttribute<EntityDeletedAttribute>();

        CollectionModifyJob *job = new CollectionModifyJob(col, q);
        q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));

        ItemFetchJob *itemFetchJob = new ItemFetchJob(col, q);
        itemFetchJob->fetchScope().fetchAttribute<EntityDeletedAttribute> (true);
        q->connect(itemFetchJob, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
        q->connect(itemFetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(removeAttribute(Akonadi::Item::List)));
    }
}

void TrashRestoreJob::TrashRestoreJobPrivate::removeAttribute(const Akonadi::Item::List &list)
{
    Q_Q(TrashRestoreJob);
    Item::List items = list;
    QMutableListIterator<Item> i(items);
    while (i.hasNext()) {
        Item &item = i.next();
        item.removeAttribute<EntityDeletedAttribute>();
        ItemModifyJob *job = new ItemModifyJob(item, q);
        job->setIgnorePayload(true);
        q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
    }
    //For some reason it is not possible to apply this change to multiple items at once
    //ItemModifyJob *job = new ItemModifyJob(items, q);
    //q->connect( job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)) );
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

void TrashRestoreJob::setTargetCollection(const Akonadi::Collection collection)
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

    //We always have to fetch the entities to ensure that the EntityDeletedAttribute is available
    if (!d->mItems.isEmpty()) {
        ItemFetchJob *job = new ItemFetchJob(d->mItems, this);
        job->fetchScope().setCacheOnly(true);
        job->fetchScope().fetchAttribute<EntityDeletedAttribute> (true);
        connect(job, SIGNAL(itemsReceived(Akonadi::Item::List)), this, SLOT(itemsReceived(Akonadi::Item::List)));
    } else if (d->mCollection.isValid()) {
        CollectionFetchJob *job = new CollectionFetchJob(d->mCollection, CollectionFetchJob::Base, this);
        connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), this, SLOT(collectionsReceived(Akonadi::Collection::List)));
    } else {
        qWarning() << "No valid collection or empty itemlist";
        setError(Job::Unknown);
        setErrorText(i18n("No valid collection or empty itemlist"));
        emitResult();
    }

}

#include "moc_trashrestorejob.cpp"
