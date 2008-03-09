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

#include <kdebug.h>

using namespace Akonadi;

TransactionBeginJob::TransactionBeginJob(QObject * parent) :
  Job( parent )
{
  Q_ASSERT( parent );
}

TransactionBeginJob::~ TransactionBeginJob()
{
}

void TransactionBeginJob::doStart()
{
  writeData( newTag() + " BEGIN\n" );
}



TransactionRollbackJob::TransactionRollbackJob(QObject * parent) :
    Job( parent )
{
  Q_ASSERT( parent );
}

TransactionRollbackJob::~ TransactionRollbackJob()
{
}

void TransactionRollbackJob::doStart()
{
  writeData( newTag() + " ROLLBACK\n" );
}



TransactionCommitJob::TransactionCommitJob(QObject * parent) :
    Job( parent )
{
  Q_ASSERT( parent );
}

TransactionCommitJob::~ TransactionCommitJob()
{
}

void TransactionCommitJob::doStart()
{
  writeData( newTag() + " COMMIT\n" );
}



class TransactionSequence::Private
{
  public:
    Private( TransactionSequence *parent ) :
      q( parent ),
      state( Idle )
    {
    }

    enum TransactionState
    {
      Idle,
      Running,
      WaitingForSubjobs,
      RollingBack,
      Committing
    };

    TransactionSequence *q;
    TransactionState state;

    void commitResult( KJob *job )
    {
      if ( job->error() ) {
        q->setError( job->error() );
        q->setErrorText( job->errorText() );
      }
      q->emitResult();
    }

    void rollbackResult( KJob *job )
    {
      Q_UNUSED( job );
      q->emitResult();
    }
};

TransactionSequence::TransactionSequence( QObject * parent ) :
    Job( parent ),
    d( new Private( this ) )
{
}

TransactionSequence::~ TransactionSequence()
{
  delete d;
}

bool TransactionSequence::addSubjob(KJob * job)
{
  if ( d->state == Private::Idle ) {
    d->state = Private::Running;
    new TransactionBeginJob( this );
  }
  return Job::addSubjob( job );
}

void TransactionSequence::slotResult(KJob * job)
{
  if ( !job->error() ) {
    Job::slotResult( job );
    if ( subjobs().isEmpty() && d->state == Private::WaitingForSubjobs ) {
      d->state = Private::Committing;
      TransactionCommitJob *job = new TransactionCommitJob( this );
      connect( job, SIGNAL(result(KJob*)), SLOT(commitResult(KJob*)) );
    }
  } else {
    setError( job->error() );
    setErrorText( job->errorText() );
    removeSubjob( job );
    clearSubjobs();
    if ( d->state == Private::Running || d->state == Private::WaitingForSubjobs ) {
      d->state = Private::RollingBack;
      TransactionRollbackJob *job = new TransactionRollbackJob( this );
      connect( job, SIGNAL(result(KJob*)), SLOT(rollbackResult(KJob*)) );
    }
  }
}

void TransactionSequence::commit()
{
  if ( d->state < Private::WaitingForSubjobs )
    d->state = Private::WaitingForSubjobs;
  else
    return;

  if ( subjobs().isEmpty() ) {
    if ( !error() ) {
      d->state = Private::Committing;
      TransactionCommitJob *job = new TransactionCommitJob( this );
      connect( job, SIGNAL(result(KJob*)), SLOT(commitResult(KJob*)) );
    } else {
      d->state = Private::RollingBack;
      TransactionRollbackJob *job = new TransactionRollbackJob( this );
      connect( job, SIGNAL(result(KJob*)), SLOT(rollbackResult(KJob*)) );
    }
  }
}

void TransactionSequence::doStart()
{
  if ( d->state == Private::Idle )
    emitResult();
  else
    commit();
}

#include "transactionjobs.moc"
