/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "subscriptionjob_p.h"

#include "job_p.h"
#include "helper_p.h"
#include "collectionmodifyjob.h"

using namespace Akonadi;

class Akonadi::SubscriptionJobPrivate : public JobPrivate
{
public:
    SubscriptionJobPrivate(SubscriptionJob *parent)
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

    for (Collection col : qAsConst(d->mSub)) {   //krazy:exclude=foreach
        col.setEnabled(true);
        new CollectionModifyJob(col, this);
    }
    for (Collection col : qAsConst(d->mUnsub)) { //krazy:exclude=foreach
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
