/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "transactionjobs.h"

#include "job_p.h"
#include "private/protocol_p.h"

using namespace Akonadi;

class Akonadi::TransactionJobPrivate : public JobPrivate
{
public:
    explicit TransactionJobPrivate(Job *parent)
        : JobPrivate(parent)
    {
    }
};

TransactionJob::TransactionJob(QObject *parent)
    : Job(new TransactionJobPrivate(this), parent)
{
    Q_ASSERT(parent);
}

TransactionJob::~TransactionJob()
{
}

void TransactionJob::doStart()
{
    Q_D(TransactionJob);

    Protocol::TransactionCommand::Mode mode;
    if (qobject_cast<TransactionBeginJob *>(this)) {
        mode = Protocol::TransactionCommand::Begin;
    } else if (qobject_cast<TransactionRollbackJob *>(this)) {
        mode = Protocol::TransactionCommand::Rollback;
    } else if (qobject_cast<TransactionCommitJob *>(this)) {
        mode = Protocol::TransactionCommand::Commit;
    } else {
        Q_ASSERT(false);
        mode = Protocol::TransactionCommand::Invalid;
    }

    d->sendCommand(Protocol::TransactionCommandPtr::create(mode));
}

bool TransactionJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::Transaction) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

TransactionBeginJob::TransactionBeginJob(QObject *parent)
    : TransactionJob(parent)
{
}

TransactionBeginJob::~TransactionBeginJob()
{
}

TransactionRollbackJob::TransactionRollbackJob(QObject *parent)
    : TransactionJob(parent)
{
}

TransactionRollbackJob::~TransactionRollbackJob()
{
}

TransactionCommitJob::TransactionCommitJob(QObject *parent)
    : TransactionJob(parent)
{
}

TransactionCommitJob::~TransactionCommitJob()
{
}

#include "moc_transactionjobs.cpp"
