/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    explicit InvalidateCacheJobPrivate(InvalidateCacheJob *qq)
        : JobPrivate(qq)
    {
    }
    Collection collection;

    QString jobDebuggingString() const override;
    void collectionFetchResult(KJob *job);
    void itemFetchResult(KJob *job);
    void itemStoreResult(KJob *job);

    Q_DECLARE_PUBLIC(InvalidateCacheJob)
};

QString InvalidateCacheJobPrivate::jobDebuggingString() const
{
    return QStringLiteral("Invalidate Cache from collection id: %1").arg(collection.id());
}

} // namespace Akonadi

void InvalidateCacheJobPrivate::collectionFetchResult(KJob *job)
{
    Q_Q(InvalidateCacheJob);
    if (job->error()) {
        return; // handled by KCompositeJob
    }

    auto *fetchJob = qobject_cast<CollectionFetchJob *>(job);
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

    auto *itemFetch = new ItemFetchJob(collection, q);
    QObject::connect(itemFetch, &ItemFetchJob::result, q, [this](KJob* job) { itemFetchResult(job);} );
}

void InvalidateCacheJobPrivate::itemFetchResult(KJob *job)
{
    Q_Q(InvalidateCacheJob);
    if (job->error()) {
        return;
    }
    auto *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob);
    if (fetchJob->items().isEmpty()) {
        q->emitResult();
        return;
    }

    ItemModifyJob *modJob = nullptr;
    const Akonadi::Item::List itemsLst = fetchJob->items();
    for (Item item : itemsLst) {
        item.clearPayload();
        modJob = new ItemModifyJob(item, q);
    }
    QObject::connect(modJob, &KJob::result, q, [this](KJob *job) { itemStoreResult(job); });
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
    auto *job = new CollectionFetchJob(d->collection, Akonadi::CollectionFetchJob::Base, this);
    connect(job, &KJob::result, this, [d](KJob *job) { d->collectionFetchResult(job); });
}

#include "moc_invalidatecachejob_p.cpp"
