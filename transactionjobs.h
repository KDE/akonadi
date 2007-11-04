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

#ifndef AKONADI_TRANSACTIONJOBS_H
#define AKONADI_TRANSACTIONJOBS_H

#include "libakonadi_export.h"
#include <libakonadi/job.h>

namespace Akonadi {

/**
  Begins a session-global transaction.
  Note: this will only have an effect when used as a subjob or with a Session.
*/
class AKONADI_EXPORT TransactionBeginJob : public Job
{
  public:
    /**
      Creates a new TransactionBeginJob.
      @param parent The parent job or Session, must not be 0.
    */
    explicit TransactionBeginJob( QObject *parent );

    /**
      Destroys this job.
    */
    ~TransactionBeginJob();

  protected:
    virtual void doStart();
};


/**
  Aborts a session-global transaction.
  Note: this will only have an effect when used as a subjob or with a Session.
*/
class AKONADI_EXPORT TransactionRollbackJob : public Job
{
  public:
    /**
      Creates a new TransactionRollbackJob.
      @param parent The parent job or Session, must not be 0.
    */
    explicit TransactionRollbackJob( QObject *parent );

    /**
      Destroys this TransactionRollbackJob.
    */
    ~TransactionRollbackJob();

  protected:
    virtual void doStart();
};


/**
  Commits a session-global transaction.
*/
class AKONADI_EXPORT TransactionCommitJob : public Job
{
  public:
    /**
      Creates a new TransactionCommitJob.
      @param parent The parent job or Session, must not be 0.
     */
    explicit TransactionCommitJob( QObject *parent );

    /**
      Destroys this TransactionCommitJob.
    */
    ~TransactionCommitJob();

  protected:
    virtual void doStart();
};


/**
  Base class for jobs that need to run a sequence of sub-jobs in a transaction.
  As soon as the first subjob is added, the transaction is started.
  As soon as the last subjob has successfully finished, the transaction is committed.
  If any subjob fails, the transaction is rolled back.
*/
class AKONADI_EXPORT TransactionSequence : public Job
{
  Q_OBJECT
  public:
    /**
      Creates a new transaction job sequence.
      @param parent The parent object.
    */
    explicit TransactionSequence( QObject *parent = 0 );

    /**
      Destroys this job.
    */
    ~TransactionSequence();

  protected:
    /**
      Commit the transaction as soon as all pending sub-jobs finished successfully.
    */
    void commit();

    bool addSubjob( KJob* job );

  protected Q_SLOTS:
    void slotResult( KJob *job );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void commitResult(KJob*) )
    Q_PRIVATE_SLOT( d, void rollbackResult(KJob*) )
};

}

#endif
