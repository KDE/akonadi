/*
    SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "trashjob.h"

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

class Akonadi::TrashJobPrivate : public JobPrivate
{
public:
    explicit TrashJobPrivate(TrashJob *parent)
        : JobPrivate(parent)
    {
    }
    // 4.
    void selectResult(KJob *job);
    // 3.
    // Helper functions to recursively set the attribute on deleted collections
    void setAttribute(const Akonadi::Collection::List & /*list*/);
    void setAttribute(const Akonadi::Item::List & /*list*/);
    // Set attributes after ensuring that move job was successful
    void setAttribute(KJob *job);

    // 2.
    // called after parent of the trashed item was fetched (needed to see in which resource the item is in)
    void parentCollectionReceived(const Akonadi::Collection::List & /*collections*/);

    // 1.
    // called after initial fetch of trashed items
    void itemsReceived(const Akonadi::Item::List & /*items*/);
    // called after initial fetch of trashed collection
    void collectionsReceived(const Akonadi::Collection::List & /*collections*/);

    Q_DECLARE_PUBLIC(TrashJob)

    Item::List mItems;
    Collection mCollection;
    Collection mRestoreCollection;
    Collection mTrashCollection;
    bool mKeepTrashInCollection = false;
    bool mSetRestoreCollection = false; // only set restore collection when moved to trash collection (not in place)
    bool mDeleteIfInTrash = false;
    QHash<Collection, Item::List> mCollectionItems; // list of trashed items sorted according to parent collection
    QHash<Item::Id, Collection> mParentCollections; // fetched parent collection of items (containing the resource name)
};

void TrashJobPrivate::selectResult(KJob *job)
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

void TrashJobPrivate::setAttribute(const Akonadi::Collection::List &list)
{
    Q_Q(TrashJob);
    QVectorIterator<Collection> i(list);
    while (i.hasNext()) {
        const Collection &col = i.next();
        auto eda = new EntityDeletedAttribute();
        if (mSetRestoreCollection) {
            Q_ASSERT(mRestoreCollection.isValid());
            eda->setRestoreCollection(mRestoreCollection);
        }

        Collection modCol(col.id()); // really only modify attribute (forget old remote ids, etc.), otherwise we have an error because of the move
        modCol.addAttribute(eda);

        auto job = new CollectionModifyJob(modCol, q);
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });

        auto itemFetchJob = new ItemFetchJob(col, q);
        // TODO not sure if it is guaranteed that itemsReceived is always before result (otherwise the result is emitted before the attributes are set)
        q->connect(itemFetchJob, &ItemFetchJob::itemsReceived, q, [this](const auto &items) {
            setAttribute(items);
        });
        q->connect(itemFetchJob, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
    }
}

void TrashJobPrivate::setAttribute(const Akonadi::Item::List &list)
{
    Q_Q(TrashJob);
    Item::List items = list;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QMutableVectorIterator<Item> i(items);
#else
    QMutableListIterator<Item> i(items);
#endif
    while (i.hasNext()) {
        const Item &item = i.next();
        auto eda = new EntityDeletedAttribute();
        if (mSetRestoreCollection) {
            // When deleting a collection, we want to restore the deleted collection's items restored to the deleted collection's parent, not the items parent
            if (mRestoreCollection.isValid()) {
                eda->setRestoreCollection(mRestoreCollection);
            } else {
                Q_ASSERT(mParentCollections.contains(item.parentCollection().id()));
                eda->setRestoreCollection(mParentCollections.value(item.parentCollection().id()));
            }
        }

        Item modItem(item.id()); // really only modify attribute (forget old remote ids, etc.)
        modItem.addAttribute(eda);
        auto job = new ItemModifyJob(modItem, q);
        job->setIgnorePayload(true);
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
    }

    // For some reason it is not possible to apply this change to multiple items at once
    /*ItemModifyJob *job = new ItemModifyJob(items, q);
    q->connect( job, SIGNAL(result(KJob*)), SLOT(selectResult(KJob*)) );*/
}

void TrashJobPrivate::setAttribute(KJob *job)
{
    Q_Q(TrashJob);
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->objectName();
        qCWarning(AKONADICORE_LOG) << job->errorString();
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Move to trash collection failed, aborting trash operation"));
        return;
    }

    // For Items
    const QVariant var = job->property("MovedItems");
    if (var.isValid()) {
        int id = var.toInt();
        Q_ASSERT(id >= 0);
        setAttribute(mCollectionItems.value(Collection(id)));
        return;
    }

    // For a collection
    Q_ASSERT(mCollection.isValid());
    setAttribute(Collection::List() << mCollection);
    // Set the attribute on all subcollections and items
    auto colFetchJob = new CollectionFetchJob(mCollection, CollectionFetchJob::Recursive, q);
    q->connect(colFetchJob, &CollectionFetchJob::collectionsReceived, q, [this](const auto &cols) {
        setAttribute(cols);
    });
    q->connect(colFetchJob, &KJob::result, q, [this](KJob *job) {
        selectResult(job);
    });
}

void TrashJobPrivate::parentCollectionReceived(const Akonadi::Collection::List &collections)
{
    Q_Q(TrashJob);
    Q_ASSERT(collections.size() == 1);
    const Collection &parentCollection = collections.first();

    // store attribute
    Q_ASSERT(!parentCollection.resource().isEmpty());
    Collection trashCollection = mTrashCollection;
    if (!mTrashCollection.isValid()) {
        trashCollection = TrashSettings::getTrashCollection(parentCollection.resource());
    }
    if (!mKeepTrashInCollection && trashCollection.isValid()) { // Only set the restore collection if the item is moved to trash
        mSetRestoreCollection = true;
    }

    mParentCollections.insert(parentCollection.id(), parentCollection);

    if (trashCollection.isValid()) { // Move the items to the correct collection if available
        auto job = new ItemMoveJob(mCollectionItems.value(parentCollection), trashCollection, q);
        job->setProperty("MovedItems", parentCollection.id());
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            setAttribute(job);
        }); // Wait until the move finished to set the attribute
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
    } else {
        setAttribute(mCollectionItems.value(parentCollection));
    }
}

void TrashJobPrivate::itemsReceived(const Akonadi::Item::List &items)
{
    Q_Q(TrashJob);
    if (items.isEmpty()) {
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Invalid items passed"));
        q->emitResult();
        return;
    }

    Item::List toDelete;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QVectorIterator<Item> i(items);
#else
    QListIterator<Item> i(items);
#endif
    while (i.hasNext()) {
        const Item &item = i.next();
        if (item.hasAttribute<EntityDeletedAttribute>()) {
            toDelete.append(item);
            continue;
        }
        Q_ASSERT(item.parentCollection().isValid());
        mCollectionItems[item.parentCollection()].append(item); // Sort by parent col ( = restore collection)
    }

    for (auto it = mCollectionItems.cbegin(), e = mCollectionItems.cend(); it != e; ++it) {
        auto job = new CollectionFetchJob(it.key(), Akonadi::CollectionFetchJob::Base, q);
        q->connect(job, &CollectionFetchJob::collectionsReceived, q, [this](const auto &cols) {
            parentCollectionReceived(cols);
        });
    }

    if (mDeleteIfInTrash && !toDelete.isEmpty()) {
        auto job = new ItemDeleteJob(toDelete, q);
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
    } else if (mCollectionItems.isEmpty()) { // No job started, so we abort the job
        qCWarning(AKONADICORE_LOG) << "Nothing to do";
        q->emitResult();
    }
}

void TrashJobPrivate::collectionsReceived(const Akonadi::Collection::List &collections)
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

    if (mCollection.hasAttribute<EntityDeletedAttribute>()) { // marked as deleted
        if (mDeleteIfInTrash) {
            auto job = new CollectionDeleteJob(mCollection, q);
            q->connect(job, &KJob::result, q, [this](KJob *job) {
                selectResult(job);
            });
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
    if (!mKeepTrashInCollection && trashCollection.isValid()) { // only set the restore collection if the item is moved to trash
        mSetRestoreCollection = true;
        Q_ASSERT(mCollection.parentCollection().isValid());
        mRestoreCollection = mCollection.parentCollection();
        mRestoreCollection.setResource(mCollection.resource()); // The parent collection doesn't contain the resource, so we have to set it manually
    }

    if (trashCollection.isValid()) {
        auto job = new CollectionMoveJob(mCollection, trashCollection, q);
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            setAttribute(job);
        });
        q->connect(job, &KJob::result, q, [this](KJob *job) {
            selectResult(job);
        });
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

    // Fetch items first to ensure that the EntityDeletedAttribute is available
    if (!d->mItems.isEmpty()) {
        auto job = new ItemFetchJob(d->mItems, this);
        job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent); // so we have access to the resource
        // job->fetchScope().setCacheOnly(true);
        job->fetchScope().fetchAttribute<EntityDeletedAttribute>(true);
        connect(job, &ItemFetchJob::itemsReceived, this, [d](const auto &items) {
            d->itemsReceived(items);
        });

    } else if (d->mCollection.isValid()) {
        auto job = new CollectionFetchJob(d->mCollection, CollectionFetchJob::Base, this);
        job->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::Parent);
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

#include "moc_trashjob.cpp"
