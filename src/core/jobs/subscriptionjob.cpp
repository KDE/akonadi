/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "subscriptionjob_p.h"

#include "job_p.h"

#include "collectionmodifyjob.h"

using namespace Akonadi;

class Akonadi::SubscriptionJobPrivate : public JobPrivate
{
public:
    explicit SubscriptionJobPrivate(SubscriptionJob *parent)
        : JobPrivate(parent)
    {
    }

    Q_DECLARE_PUBLIC(SubscriptionJob)

    Collection::List mSub, mUnsub;
};

SubscriptionJob::SubscriptionJob(QObject *parent)
    : Job(new SubscriptionJobPrivate(this), parent)
{
}

SubscriptionJob::~SubscriptionJob()
{
}

void SubscriptionJob::subscribe(const Collection::List &list)
{
    Q_D(SubscriptionJob);

    d->mSub = list;
}

void SubscriptionJob::unsubscribe(const Collection::List &list)
{
    Q_D(SubscriptionJob);

    d->mUnsub = list;
}

void SubscriptionJob::doStart()
{
    Q_D(SubscriptionJob);

    if (d->mSub.isEmpty() && d->mUnsub.isEmpty()) {
        emitResult();
        return;
    }

    for (Collection col : std::as_const(d->mSub)) {
        col.setEnabled(true);
        new CollectionModifyJob(col, this);
    }
    for (Collection col : std::as_const(d->mUnsub)) {
        col.setEnabled(false);
        new CollectionModifyJob(col, this);
    }
}

void SubscriptionJob::slotResult(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        Q_FOREACH (KJob *subjob, subjobs()) {
            removeSubjob(subjob);
        }
        emitResult();
    } else {
        Job::slotResult(job);

        if (!hasSubjobs()) {
            emitResult();
        }
    }
}

#include "moc_subscriptionjob_p.cpp"
