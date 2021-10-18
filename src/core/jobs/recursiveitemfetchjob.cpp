/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "recursiveitemfetchjob.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"

#include <QStringList>

using namespace Akonadi;

class Akonadi::RecursiveItemFetchJobPrivate
{
public:
    RecursiveItemFetchJobPrivate(const Collection &collection, const QStringList &mimeTypes, RecursiveItemFetchJob *parent)
        : mParent(parent)
        , mCollection(collection)
        , mMimeTypes(mimeTypes)
    {
    }

    void collectionFetchResult(KJob *job)
    {
        if (job->error()) {
            mParent->emitResult();
            return;
        }

        const CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob *>(job);

        Collection::List collections = fetchJob->collections();
        collections.prepend(mCollection);

        for (const Collection &collection : std::as_const(collections)) {
            auto itemFetchJob = new ItemFetchJob(collection, mParent);
            itemFetchJob->setFetchScope(mFetchScope);
            mParent->connect(itemFetchJob, &KJob::result, mParent, [this](KJob *job) {
                itemFetchResult(job);
            });

            mFetchCount++;
        }
    }

    void itemFetchResult(KJob *job)
    {
        if (!job->error()) {
            const ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);

            if (!mMimeTypes.isEmpty()) {
                const Akonadi::Item::List lstItems = fetchJob->items();
                for (const Item &item : lstItems) {
                    if (mMimeTypes.contains(item.mimeType())) {
                        mItems << item;
                    }
                }
            } else {
                mItems << fetchJob->items();
            }
        }

        mFetchCount--;

        if (mFetchCount == 0) {
            mParent->emitResult();
        }
    }

    RecursiveItemFetchJob *const mParent;
    const Collection mCollection;
    Item::List mItems;
    ItemFetchScope mFetchScope;
    const QStringList mMimeTypes;

    int mFetchCount = 0;
};

RecursiveItemFetchJob::RecursiveItemFetchJob(const Collection &collection, const QStringList &mimeTypes, QObject *parent)
    : KJob(parent)
    , d(new RecursiveItemFetchJobPrivate(collection, mimeTypes, this))
{
}

RecursiveItemFetchJob::~RecursiveItemFetchJob() = default;

void RecursiveItemFetchJob::setFetchScope(const ItemFetchScope &fetchScope)
{
    d->mFetchScope = fetchScope;
}

ItemFetchScope &RecursiveItemFetchJob::fetchScope()
{
    return d->mFetchScope;
}

void RecursiveItemFetchJob::start()
{
    auto job = new CollectionFetchJob(d->mCollection, CollectionFetchJob::Recursive, this);

    if (!d->mMimeTypes.isEmpty()) {
        job->fetchScope().setContentMimeTypes(d->mMimeTypes);
    }

    connect(job, &CollectionFetchJob::result, this, [this](KJob *job) {
        d->collectionFetchResult(job);
    });
}

Akonadi::Item::List RecursiveItemFetchJob::items() const
{
    return d->mItems;
}

#include "moc_recursiveitemfetchjob.cpp"
