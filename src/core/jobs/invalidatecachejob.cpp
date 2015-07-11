/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include "invalidatecachejob_p.h"
#include "job_p.h"
#include "collectionfetchjob.h"
#include "itemfetchjob.h"
#include "itemmodifyjob.h"

#include <KLocalizedString>

using namespace Akonadi;

namespace Akonadi
{

class InvalidateCacheJobPrivate : JobPrivate
{
public:
    InvalidateCacheJobPrivate(InvalidateCacheJob *qq)
        : JobPrivate(qq)
    {
    }
    Collection collection;

    void collectionFetchResult(KJob *job);
    void itemFetchResult(KJob *job);
    void itemStoreResult(KJob *job);

    Q_DECLARE_PUBLIC(InvalidateCacheJob)
};

}

void InvalidateCacheJobPrivate::collectionFetchResult(KJob *job)
{
    Q_Q(InvalidateCacheJob);
    if (job->error()) {
        return; // handled by KCompositeJob
    }

    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob *>(job);
    Q_ASSERT(fetchJob);
    if (fetchJob->collections().size() == 1) {
        collection = fetchJob->collections().at(0);
    }

    if (!collection.isValid()) {
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Invalid collection."));
        q->emitResult();
        return;
    }

    ItemFetchJob *itemFetch = new ItemFetchJob(collection, q);
    QObject::connect(itemFetch, SIGNAL(result(KJob*)), q, SLOT(itemFetchResult(KJob*)));
}

void InvalidateCacheJobPrivate::itemFetchResult(KJob *job)
{
    Q_Q(InvalidateCacheJob);
    if (job->error()) {
        return;
    }
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob);
    if (fetchJob->items().size() == 0) {
        q->emitResult();
        return;
    }

    ItemModifyJob *modJob = 0;
    foreach (Item item, fetchJob->items()) {    //krazy:exclude=foreach, item cannot be const
        item.clearPayload();
        modJob = new ItemModifyJob(item, q);
    }
    QObject::connect(modJob, SIGNAL(result(KJob*)), q, SLOT(itemStoreResult(KJob*)));
}

void InvalidateCacheJobPrivate::itemStoreResult(KJob *job)
{
    Q_Q(InvalidateCacheJob);
    if (job->error()) {
        return;
    }
    q->emitResult();
}

InvalidateCacheJob::InvalidateCacheJob(const Collection &collection, QObject *parent)
    : Job(new InvalidateCacheJobPrivate(this), parent)
{
    Q_D(InvalidateCacheJob);
    d->collection = collection;
}

void InvalidateCacheJob::doStart()
{
    Q_D(InvalidateCacheJob);
    // resolve RID-only collections
    CollectionFetchJob *job = new CollectionFetchJob(d->collection, Akonadi::CollectionFetchJob::Base, this);
    connect(job, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)));
}

#include "moc_invalidatecachejob_p.cpp"
