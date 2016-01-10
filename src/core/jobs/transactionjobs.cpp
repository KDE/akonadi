/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "transactionjobs.h"

#include "job_p.h"
#include "private/protocol_p.h"

using namespace Akonadi;

class Akonadi::TransactionJobPrivate : public JobPrivate
{
public:
    TransactionJobPrivate(Job *parent)
        : JobPrivate(parent)
    {}
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

    d->sendCommand(Protocol::TransactionCommand(mode));
}

bool TransactionJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    if (!response.isResponse() || response.type() != Protocol::Command::Transaction) {
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
