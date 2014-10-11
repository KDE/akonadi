/*
    Copyright (c) 2006-2008 Volker Krause <vkrause@kde.org>

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

#include "transactionsequence.h"
#include "transactionjobs.h"

#include "job_p.h"

#include <QtCore/QSet>
#include <QtCore/QVariant>

using namespace Akonadi;

class Akonadi::TransactionSequencePrivate : public JobPrivate
{
public:
    TransactionSequencePrivate(TransactionSequence *parent)
        : JobPrivate(parent)
        , mState(Idle)
        , mAutoCommit(true)
    {
    }

    enum TransactionState {
        Idle,
        Running,
        WaitingForSubjobs,
        RollingBack,
        Committing
    };

    Q_DECLARE_PUBLIC(TransactionSequence)

    TransactionState mState;
    QSet<KJob *> mIgnoredErrorJobs;
    bool mAutoCommit;

    void commitResult(KJob *job)
    {
        Q_Q(TransactionSequence);

        if (job->error()) {
            q->setError(job->error());
            q->setErrorText(job->errorText());
        }
        q->emitResult();
    }

    void rollbackResult(KJob *job)
    {
        Q_Q(TransactionSequence);

        Q_UNUSED(job);
        q->emitResult();
    }
};

TransactionSequence::TransactionSequence(QObject *parent)
    : Job(new TransactionSequencePrivate(this), parent)
{
}

TransactionSequence::~TransactionSequence()
{
}

bool TransactionSequence::addSubjob(KJob *job)
{
    Q_D(TransactionSequence);

    //Don't abort the rollback job, while keeping the state set.
    if (d->mState == TransactionSequencePrivate::RollingBack) {
        return Job::addSubjob(job);
    }

    if (error()) {
        //This can happen if a rollback is in progress, so make sure we don't set the state back to running.
        job->kill(EmitResult);
        return false;
    }
    // TODO KDE5: remove property hack once SpecialCollectionsRequestJob has been fixed
    if (d->mState == TransactionSequencePrivate::Idle && !property("transactionsDisabled").toBool()) {
        d->mState = TransactionSequencePrivate::Running; // needs to be set before creating the transaction job to avoid infinite recursion
        new TransactionBeginJob(this);
    } else {
        d->mState = TransactionSequencePrivate::Running;
    }
    return Job::addSubjob(job);
}

void TransactionSequence::slotResult(KJob *job)
{
    Q_D(TransactionSequence);

    if (!job->error() || d->mIgnoredErrorJobs.contains(job)) {
        // If we have an error but want to ignore it, we can't call Job::slotResult
        // because it would confuse the subjob queue processing logic. Just removing
        // the subjob instead is fine.
        if (!job->error()) {
            Job::slotResult(job);
        } else {
            Job::removeSubjob(job);
        }

        if (!hasSubjobs() && d->mState == TransactionSequencePrivate::WaitingForSubjobs) {
            if (property("transactionsDisabled").toBool()) {
                emitResult();
                return;
            }
            d->mState = TransactionSequencePrivate::Committing;
            TransactionCommitJob *job = new TransactionCommitJob(this);
            connect(job, SIGNAL(result(KJob*)), SLOT(commitResult(KJob*)));
        }
    } else {
        setError(job->error());
        setErrorText(job->errorText());
        removeSubjob(job);

        // cancel all subjobs in case someone else is listening (such as ItemSync), but without notifying ourselves again
        foreach (KJob *job, subjobs()) {
            disconnect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));
            job->kill(EmitResult);
        }
        clearSubjobs();

        if (d->mState == TransactionSequencePrivate::Running || d->mState == TransactionSequencePrivate::WaitingForSubjobs) {
            if (property("transactionsDisabled").toBool()) {
                emitResult();
                return;
            }
            d->mState = TransactionSequencePrivate::RollingBack;
            TransactionRollbackJob *job = new TransactionRollbackJob(this);
            connect(job, SIGNAL(result(KJob*)), SLOT(rollbackResult(KJob*)));
        }
    }
}

void TransactionSequence::commit()
{
    Q_D(TransactionSequence);

    if (d->mState == TransactionSequencePrivate::Running) {
        d->mState = TransactionSequencePrivate::WaitingForSubjobs;
    } else {
        // we never got any subjobs, that means we never started a transaction
        // so we can just quit here
        if (d->mState == TransactionSequencePrivate::Idle) {
            emitResult();
        }
        return;
    }

    if (subjobs().isEmpty()) {
        if (property("transactionsDisabled").toBool()) {
            emitResult();
            return;
        }
        if (!error()) {
            d->mState = TransactionSequencePrivate::Committing;
            TransactionCommitJob *job = new TransactionCommitJob(this);
            connect(job, SIGNAL(result(KJob*)), SLOT(commitResult(KJob*)));
        } else {
            d->mState = TransactionSequencePrivate::RollingBack;
            TransactionRollbackJob *job = new TransactionRollbackJob(this);
            connect(job, SIGNAL(result(KJob*)), SLOT(rollbackResult(KJob*)));
        }
    }
}

void TransactionSequence::setIgnoreJobFailure(KJob *job)
{
    Q_D(TransactionSequence);

    // make sure this is one of our sub jobs
    Q_ASSERT(subjobs().contains(job));

    d->mIgnoredErrorJobs.insert(job);
}

void TransactionSequence::doStart()
{
    Q_D(TransactionSequence);

    if (d->mAutoCommit) {
        if (d->mState == TransactionSequencePrivate::Idle) {
            emitResult();
        } else {
            commit();
        }
    }
}

void TransactionSequence::setAutomaticCommittingEnabled(bool enable)
{
    Q_D(TransactionSequence);
    d->mAutoCommit = enable;
}

void TransactionSequence::rollback()
{
    Q_D(TransactionSequence);

    setError(UserCanceled);
    // we never really started
    if (d->mState == TransactionSequencePrivate::Idle) {
        emitResult();
        return;
    }

    // TODO cancel not yet executed sub-jobs here

    d->mState = TransactionSequencePrivate::RollingBack;
    TransactionRollbackJob *job = new TransactionRollbackJob(this);
    connect(job, SIGNAL(result(KJob*)), SLOT(rollbackResult(KJob*)));
}

#include "moc_transactionsequence.cpp"
