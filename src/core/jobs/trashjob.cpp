/*
    Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#include "trashjob.h"

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
#include "itemfetchscope.h"
#include "collectionfetchscope.h"
#include "itemfetchjob.h"
#include "collectionfetchjob.h"

#include "akonadicore_debug.h"

#include <QHash>

using namespace Akonadi;

class TrashJob::TrashJobPrivate : public JobPrivate
{
public:
    TrashJobPrivate(TrashJob *parent)
        : JobPrivate(parent)
        , mKeepTrashInCollection(false)
        , mSetRestoreCollection(false)
        , mDeleteIfInTrash(false)
    {
    }
//4.
    void selectResult(KJob *job);
//3.
    //Helper functions to recursively set the attribute on deleted collections
    void setAttribute(const Akonadi::Collection::List &);
    void setAttribute(const Akonadi::Item::List &);
    //Set attributes after ensuring that move job was successful
    void setAttribute(KJob *job);

//2.
    //called after parent of the trashed item was fetched (needed to see in which resource the item is in)
    void parentCollectionReceived(const Akonadi::Collection::List &);

//1.
    //called after initial fetch of trashed items
    void itemsReceived(const Akonadi::Item::List &);
    //called after initial fetch of trashed collection
    void collectionsReceived(const Akonadi::Collection::List &);

    Q_DECLARE_PUBLIC(TrashJob)

    Item::List mItems;
    Collection mCollection;
    Collection mRestoreCollection;
    Collection mTrashCollection;
    bool mKeepTrashInCollection;
    bool mSetRestoreCollection; //only set restore collection when moved to trash collection (not in place)
    bool mDeleteIfInTrash;
    QHash<Collection, Item::List> mCollectionItems; //list of trashed items sorted according to parent collection
    QHash<Item::Id, Collection> mParentCollections; //fetched parent collection of items (containing the resource name)

};

void TrashJob::TrashJobPrivate::selectResult(KJob *job)
{
    Q_Q(TrashJob);
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->objectName();
        qCWarning(AKONADICORE_LOG) << job->errorString();
        return; // KCompositeJob takes care of errors
    }

    if (!q->hasSubjobs() || (q->subjobs().contains(static_cast<KJob *>(q->sender())) && q->subjobs().size() == 1)) {
        q->emitResult();
    }
}

void TrashJob::TrashJobPrivate::setAttribute(const Akonadi::Collection::List &list)
{
    Q_Q(TrashJob);
    QVectorIterator<Collection> i(list);
    while (i.hasNext()) {
        const Collection &col = i.next();
        EntityDeletedAttribute *eda = new EntityDeletedAttribute();
        if (mSetRestoreCollection) {
            Q_ASSERT(mRestoreCollection.isValid());
            eda->setRestoreCollection(mRestoreCollection);
        }

        Collection modCol(col.id());   //really only modify attribute (forget old remote ids, etc.), otherwise we have an error because of the move
        modCol.addAttribute(eda);

        CollectionModifyJob *job = new CollectionModifyJob(modCol, q);
        q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));

        ItemFetchJob *itemFetchJob = new ItemFetchJob(col, q);
        //TODO not sure if it is guaranteed that itemsReceived is always before result (otherwise the result is emitted before the attributes are set)
        q->connect(itemFetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)), SLOT(setAttribute(Akonadi::Item::List)));
        q->connect(itemFetchJob, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
    }
}

void TrashJob::TrashJobPrivate::setAttribute(const Akonadi::Item::List &list)
{
    Q_Q(TrashJob);
    Item::List items = list;
    QMutableVectorIterator<Item> i(items);
    while (i.hasNext()) {
        const Item &item = i.next();
        EntityDeletedAttribute *eda = new EntityDeletedAttribute();
        if (mSetRestoreCollection) {
            //When deleting a collection, we want to restore the deleted collection's items restored to the deleted collection's parent, not the items parent
            if (mRestoreCollection.isValid()) {
                eda->setRestoreCollection(mRestoreCollection);
            } else {
                Q_ASSERT(mParentCollections.contains(item.parentCollection().id()));
                eda->setRestoreCollection(mParentCollections.value(item.parentCollection().id()));
            }
        }

        Item modItem(item.id());   //really only modify attribute (forget old remote ids, etc.)
        modItem.addAttribute(eda);
        ItemModifyJob *job = new ItemModifyJob(modItem, q);
        job->setIgnorePayload(true);
        q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
    }

    //For some reason it is not possible to apply this change to multiple items at once
    /*ItemModifyJob *job = new ItemModifyJob(items, q);
    q->connect( job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)) );*/
}

void TrashJob::TrashJobPrivate::setAttribute(KJob *job)
{
    Q_Q(TrashJob);
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->objectName();
        qCWarning(AKONADICORE_LOG) << job->errorString();
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Move to trash collection failed, aborting trash operation"));
        return;
    }

    //For Items
    const QVariant var = job->property("MovedItems");
    if (var.isValid()) {
        int id = var.toInt();
        Q_ASSERT(id >= 0);
        setAttribute(mCollectionItems.value(Collection(id)));
        return;
    }

    //For a collection
    Q_ASSERT(mCollection.isValid());
    setAttribute(Collection::List() << mCollection);
    //Set the attribute on all subcollections and items
    CollectionFetchJob *colFetchJob = new CollectionFetchJob(mCollection, CollectionFetchJob::Recursive, q);
    q->connect(colFetchJob, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(setAttribute(Akonadi::Collection::List)));
    q->connect(colFetchJob, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
}

void TrashJob::TrashJobPrivate::parentCollectionReceived(const Akonadi::Collection::List &collections)
{
    Q_Q(TrashJob);
    Q_ASSERT(collections.size() == 1);
    const Collection &parentCollection = collections.first();

    //store attribute
    Q_ASSERT(!parentCollection.resource().isEmpty());
    Collection trashCollection = mTrashCollection;
    if (!mTrashCollection.isValid()) {
        trashCollection = TrashSettings::getTrashCollection(parentCollection.resource());
    }
    if (!mKeepTrashInCollection && trashCollection.isValid()) {   //Only set the restore collection if the item is moved to trash
        mSetRestoreCollection = true;
    }

    mParentCollections.insert(parentCollection.id(), parentCollection);

    if (trashCollection.isValid()) {    //Move the items to the correct collection if available
        ItemMoveJob *job = new ItemMoveJob(mCollectionItems.value(parentCollection), trashCollection, q);
        job->setProperty("MovedItems", parentCollection.id());
        q->connect(job, SIGNAL(result(KJob*)), SLOT(setAttribute(KJob*))); //Wait until the move finished to set the attribute
        q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
    } else {
        setAttribute(mCollectionItems.value(parentCollection));
    }
}

void TrashJob::TrashJobPrivate::itemsReceived(const Akonadi::Item::List &items)
{
    Q_Q(TrashJob);
    if (items.isEmpty()) {
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Invalid items passed"));
        q->emitResult();
        return;
    }

    Item::List toDelete;

    QVectorIterator<Item> i(items);
    while (i.hasNext()) {
        const Item &item = i.next();
        if (item.hasAttribute<EntityDeletedAttribute>()) {
            toDelete.append(item);
            continue;
        }
        Q_ASSERT(item.parentCollection().isValid());
        mCollectionItems[item.parentCollection()].append(item);   //Sort by parent col ( = restore collection)
    }

    for (auto it = mCollectionItems.cbegin(), e = mCollectionItems.cend(); it != e; ++it) {
        CollectionFetchJob *job = new CollectionFetchJob(it.key(), Akonadi::CollectionFetchJob::Base, q);
        q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                   SLOT(parentCollectionReceived(Akonadi::Collection::List)));
    }

    if (mDeleteIfInTrash && !toDelete.isEmpty()) {
        ItemDeleteJob *job = new ItemDeleteJob(toDelete, q);
        q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
    } else if (mCollectionItems.isEmpty()) {   //No job started, so we abort the job
        qCWarning(AKONADICORE_LOG) << "Nothing to do";
        q->emitResult();
    }

}

void TrashJob::TrashJobPrivate::collectionsReceived(const Akonadi::Collection::List &collections)
{
    Q_Q(TrashJob);
    if (collections.isEmpty()) {
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Invalid collection passed"));
        q->emitResult();
        return;
    }
    Q_ASSERT(collections.size() == 1);
    mCollection = collections.first();

    if (mCollection.hasAttribute<EntityDeletedAttribute>()) {  //marked as deleted
        if (mDeleteIfInTrash) {
            CollectionDeleteJob *job = new CollectionDeleteJob(mCollection, q);
            q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
        } else {
            qCWarning(AKONADICORE_LOG) << "Nothing to do";
            q->emitResult();
        }
        return;
    }

    Collection trashCollection = mTrashCollection;
    if (!mTrashCollection.isValid()) {
        trashCollection = TrashSettings::getTrashCollection(mCollection.resource());
    }
    if (!mKeepTrashInCollection && trashCollection.isValid()) {   //only set the restore collection if the item is moved to trash
        mSetRestoreCollection = true;
        Q_ASSERT(mCollection.parentCollection().isValid());
        mRestoreCollection = mCollection.parentCollection();
        mRestoreCollection.setResource(mCollection.resource());   //The parent collection doesn't contain the resource, so we have to set it manually
    }

    if (trashCollection.isValid()) {
        CollectionMoveJob *job = new CollectionMoveJob(mCollection, trashCollection, q);
        q->connect(job, SIGNAL(result(KJob*)), SLOT(setAttribute(KJob*)));
        q->connect(job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)));
    } else {
        setAttribute(Collection::List() << mCollection);
    }

}

TrashJob::TrashJob(const Item &item, QObject *parent)
    : Job(new TrashJobPrivate(this), parent)
{
    Q_D(TrashJob);
    d->mItems << item;
}

TrashJob::TrashJob(const Item::List &items, QObject *parent)
    : Job(new TrashJobPrivate(this), parent)
{
    Q_D(TrashJob);
    d->mItems = items;
}

TrashJob::TrashJob(const Collection &collection, QObject *parent)
    : Job(new TrashJobPrivate(this), parent)
{
    Q_D(TrashJob);
    d->mCollection = collection;
}

TrashJob::~TrashJob()
{
}

Item::List TrashJob::items() const
{
    Q_D(const TrashJob);
    return d->mItems;
}

void TrashJob::setTrashCollection(const Akonadi::Collection &collection)
{
    Q_D(TrashJob);
    d->mTrashCollection = collection;
}

void TrashJob::keepTrashInCollection(bool enable)
{
    Q_D(TrashJob);
    d->mKeepTrashInCollection = enable;
}

void TrashJob::deleteIfInTrash(bool enable)
{
    Q_D(TrashJob);
    d->mDeleteIfInTrash = enable;
}

void TrashJob::doStart()
{
    Q_D(TrashJob);

    //Fetch items first to ensure that the EntityDeletedAttribute is available
    if (!d->mItems.isEmpty()) {
        ItemFetchJob *job = new ItemFetchJob(d->mItems, this);
        job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);   //so we have access to the resource
        //job->fetchScope().setCacheOnly(true);
        job->fetchScope().fetchAttribute<EntityDeletedAttribute>(true);
        connect(job, SIGNAL(itemsReceived(Akonadi::Item::List)), this, SLOT(itemsReceived(Akonadi::Item::List)));

    } else if (d->mCollection.isValid()) {
        CollectionFetchJob *job = new CollectionFetchJob(d->mCollection, CollectionFetchJob::Base, this);
        job->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::Parent);
        connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), this, SLOT(collectionsReceived(Akonadi::Collection::List)));

    } else {
        qCWarning(AKONADICORE_LOG) << "No valid collection or empty itemlist";
        setError(Job::Unknown);
        setErrorText(i18n("No valid collection or empty itemlist"));
        emitResult();
    }
}

#include "moc_trashjob.cpp"
