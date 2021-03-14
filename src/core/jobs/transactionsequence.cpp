/*
    SPDX-FileCopyrightText: 2006-2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "transactionsequence.h"
#include "transactionjobs.h"

#include "job_p.h"

#include <QSet>

using namespace Akonadi;

class Akonadi::TransactionSequencePrivate : public JobPrivate
{
public:
    explicit TransactionSequencePrivate(TransactionSequence *parent)
        : JobPrivate(parent)
        , mState(Idle)
    {
    }

    enum TransactionState {
        Idle,
        Running,
        WaitingForSubjobs,
        RollingBack,
        Committing,
    };

    Q_DECLARE_PUBLIC(TransactionSequence)

    TransactionState mState;
    QSet<KJob *> mIgnoredErrorJobs;
    bool mAutoCommit = true;

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

        Q_UNUSED(job)
        q->emitResult();
    }

    QString jobDebuggingString() const override;
};

QString Akonadi::TransactionSequencePrivate::jobDebuggingString() const
{
    // TODO add state
    return QStringLiteral("autocommit %1").arg(mAutoCommit);
}

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

    // Don't abort the rollback job, while keeping the state set.
    if (d->mState == TransactionSequencePrivate::RollingBack) {
        return Job::addSubjob(job);
    }

    if (error()) {
        // This can happen if a rollback is in progress, so make sure we don't set the state back to running.
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

        if (!hasSubjobs()) {
            if (d->mState == TransactionSequencePrivate::WaitingForSubjobs) {
                if (property("transactionsDisabled").toBool()) {
                    emitResult();
                    return;
                }
                d->mState = TransactionSequencePrivate::Committing;
                auto job = new TransactionCommitJob(this);
                connect(job, &TransactionCommitJob::result, [d](KJob *job) {
                    d->commitResult(job);
                });
            }
        }
    } else if (job->error() == KJob::KilledJobError) {
        Job::slotResult(job);
    } else {
        setError(job->error());
        setErrorText(job->errorText());
        removeSubjob(job);

        // cancel all subjobs in case someone else is listening (such as ItemSync)
        const auto subjobs = this->subjobs();
        for (KJob *job : subjobs) {
            job->kill(KJob::EmitResult);
        }
        clearSubjobs();

        if (d->mState == TransactionSequencePrivate::Running || d->mState == TransactionSequencePrivate::WaitingForSubjobs) {
            if (property("transactionsDisabled").toBool()) {
                emitResult();
                return;
            }
            d->mState = TransactionSequencePrivate::RollingBack;
            auto job = new TransactionRollbackJob(this);
            connect(job, &TransactionRollbackJob::result, this, [d](KJob *job) {
                d->rollbackResult(job);
            });
        }
    }
}

void TransactionSequence::commit()
{
    Q_D(TransactionSequence);

    if (d->mState == TransactionSequencePrivate::Running) {
        d->mState = TransactionSequencePrivate::WaitingForSubjobs;
    } else if (d->mState == TransactionSequencePrivate::RollingBack) {
        return;
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
            auto job = new TransactionCommitJob(this);
            connect(job, &TransactionCommitJob::result, this, [d](KJob *job) {
                d->commitResult(job);
            });
        } else {
            d->mState = TransactionSequencePrivate::RollingBack;
            auto job = new TransactionRollbackJob(this);
            connect(job, &TransactionRollbackJob::result, this, [d](KJob *job) {
                d->rollbackResult(job);
            });
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

    const auto jobList = subjobs();
    for (KJob *job : jobList) {
        // Killing the current subjob means forcibly closing the akonadiserver socket
        // (with a bit of delay since it happens in a secondary thread)
        // which means the next job gets disconnected
        // and the itemsync finishes with error "Cannot connect to the Akonadi service.", not ideal
        if (job != d->mCurrentSubJob) {
            job->kill(KJob::EmitResult);
        }
    }

    d->mState = TransactionSequencePrivate::RollingBack;
    auto job = new TransactionRollbackJob(this);
    connect(job, &TransactionRollbackJob::result, this, [d](KJob *job) {
        d->rollbackResult(job);
    });
}

#include "moc_transactionsequence.cpp"
