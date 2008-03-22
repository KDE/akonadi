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

#include <kdebug.h>

using namespace Akonadi;

class Akonadi::TransactionBeginJobPrivate : public JobPrivate
{
  public:
    TransactionBeginJobPrivate( TransactionBeginJob *parent )
      : JobPrivate( parent )
    {
    }
};

class Akonadi::TransactionRollbackJobPrivate : public JobPrivate
{
  public:
    TransactionRollbackJobPrivate( TransactionRollbackJob *parent )
      : JobPrivate( parent )
    {
    }
};

class Akonadi::TransactionCommitJobPrivate : public JobPrivate
{
  public:
    TransactionCommitJobPrivate( TransactionCommitJob *parent )
      : JobPrivate( parent )
    {
    }
};


TransactionBeginJob::TransactionBeginJob(QObject * parent)
  : Job( new TransactionBeginJobPrivate( this ), parent )
{
  Q_ASSERT( parent );
}

TransactionBeginJob::~TransactionBeginJob()
{
  Q_D( TransactionBeginJob );

  delete d;
}

void TransactionBeginJob::doStart()
{
  writeData( newTag() + " BEGIN\n" );
}



TransactionRollbackJob::TransactionRollbackJob(QObject * parent)
  : Job( new TransactionRollbackJobPrivate( this ), parent )
{
  Q_ASSERT( parent );
}

TransactionRollbackJob::~TransactionRollbackJob()
{
  Q_D( TransactionRollbackJob );

  delete d;
}

void TransactionRollbackJob::doStart()
{
  writeData( newTag() + " ROLLBACK\n" );
}



TransactionCommitJob::TransactionCommitJob(QObject * parent)
  : Job( new TransactionCommitJobPrivate( this ), parent )
{
  Q_ASSERT( parent );
}

TransactionCommitJob::~TransactionCommitJob()
{
  Q_D( TransactionCommitJob );

  delete d;
}

void TransactionCommitJob::doStart()
{
  writeData( newTag() + " COMMIT\n" );
}



class Akonadi::TransactionSequencePrivate : public JobPrivate
{
  public:
    TransactionSequencePrivate( TransactionSequence *parent )
      : JobPrivate( parent ),
        mState( Idle )
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

    Q_DECLARE_PUBLIC( TransactionSequence )

    TransactionState mState;

    void commitResult( KJob *job )
    {
      Q_Q( TransactionSequence );

      if ( job->error() ) {
        q->setError( job->error() );
        q->setErrorText( job->errorText() );
      }
      q->emitResult();
    }

    void rollbackResult( KJob *job )
    {
      Q_Q( TransactionSequence );

      Q_UNUSED( job );
      q->emitResult();
    }
};

TransactionSequence::TransactionSequence( QObject * parent )
  : Job( new TransactionSequencePrivate( this ), parent )
{
}

TransactionSequence::~ TransactionSequence()
{
  Q_D( TransactionSequence );

  delete d;
}

bool TransactionSequence::addSubjob(KJob * job)
{
  Q_D( TransactionSequence );

  if ( d->mState == TransactionSequencePrivate::Idle ) {
    d->mState = TransactionSequencePrivate::Running;
    new TransactionBeginJob( this );
  }
  return Job::addSubjob( job );
}

void TransactionSequence::slotResult(KJob * job)
{
  Q_D( TransactionSequence );

  if ( !job->error() ) {
    Job::slotResult( job );
    if ( subjobs().isEmpty() && d->mState == TransactionSequencePrivate::WaitingForSubjobs ) {
      d->mState = TransactionSequencePrivate::Committing;
      TransactionCommitJob *job = new TransactionCommitJob( this );
      connect( job, SIGNAL(result(KJob*)), SLOT(commitResult(KJob*)) );
    }
  } else {
    setError( job->error() );
    setErrorText( job->errorText() );
    removeSubjob( job );
    clearSubjobs();
    if ( d->mState == TransactionSequencePrivate::Running || d->mState == TransactionSequencePrivate::WaitingForSubjobs ) {
      d->mState = TransactionSequencePrivate::RollingBack;
      TransactionRollbackJob *job = new TransactionRollbackJob( this );
      connect( job, SIGNAL(result(KJob*)), SLOT(rollbackResult(KJob*)) );
    }
  }
}

void TransactionSequence::commit()
{
  Q_D( TransactionSequence );

  if ( d->mState < TransactionSequencePrivate::WaitingForSubjobs )
    d->mState = TransactionSequencePrivate::WaitingForSubjobs;
  else
    return;

  if ( subjobs().isEmpty() ) {
    if ( !error() ) {
      d->mState = TransactionSequencePrivate::Committing;
      TransactionCommitJob *job = new TransactionCommitJob( this );
      connect( job, SIGNAL(result(KJob*)), SLOT(commitResult(KJob*)) );
    } else {
      d->mState = TransactionSequencePrivate::RollingBack;
      TransactionRollbackJob *job = new TransactionRollbackJob( this );
      connect( job, SIGNAL(result(KJob*)), SLOT(rollbackResult(KJob*)) );
    }
  }
}

void TransactionSequence::doStart()
{
  Q_D( TransactionSequence );

  if ( d->mState == TransactionSequencePrivate::Idle )
    emitResult();
  else
    commit();
}

#include "transactionjobs.moc"
