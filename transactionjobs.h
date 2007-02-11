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

#include <libakonadi/job.h>

namespace Akonadi {

/**
  Begins a session-global transaction.
  Note: this will only have an effect when used as a subjob or with a Session.
*/
class TransactionBeginJob : public Job
{
  public:
    /**
      Creates a new TransactionBeginJob.
      @param parent The parent job or Session, must not be 0.
    */
    TransactionBeginJob( Job *parent );

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
class TransactionRollbackJob : public Job
{
  public:
    /**
      Creates a new TransactionRollbackJob.
      @param parent The parent job or Session, must not be 0.
    */
    TransactionRollbackJob( Job *parent );

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
class TransactionCommitJob : public Job
{
  public:
    /**
      Creates a new TransactionCommitJob.
      @param parent The parent job or Session, must not be 0.
     */
    TransactionCommitJob( Job *parent );

    /**
      Destroys this TransactionCommitJob.
    */
    ~TransactionCommitJob();

  protected:
    virtual void doStart();
};

}

#endif
