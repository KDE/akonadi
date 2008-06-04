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

#include "akonadi_export.h"

#include <akonadi/job.h>

namespace Akonadi {

class TransactionBeginJobPrivate;
class TransactionRollbackJobPrivate;
class TransactionCommitJobPrivate;

/**
 * @short Job that begins a session-global transaction.
 *
 * Sometimes you want to execute a sequence of commands in
 * an atomic way, so that either all commands or none shall
 * be executed. The TransactionBeginJob, TransactionCommitJob and
 * TransactionRollbackJob provide these functionality for the
 * Akonadi Job classes.
 *
 * @note This will only have an effect when used as a subjob or with a Session.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT TransactionBeginJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a new transaction begin job.
     *
     * @param parent The parent job or Session, must not be 0.
     */
    explicit TransactionBeginJob( QObject *parent );

    /**
     * Destroys the transaction begin job.
     */
    ~TransactionBeginJob();

  protected:
    virtual void doStart();

  private:
    Q_DECLARE_PRIVATE( TransactionBeginJob )
};


/**
 * @short Job that aborts a session-global transaction.
 *
 * If a job inside a TransactionBeginJob has been failed,
 * the TransactionRollbackJob can be used to rollback all changes done by these
 * jobs.
 *
 * @note This will only have an effect when used as a subjob or with a Session.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT TransactionRollbackJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a new transaction rollback job.
     * The parent must be the same parent as for the TransactionBeginJob.
     *
     * @param parent The parent job or Session, must not be 0.
     */
    explicit TransactionRollbackJob( QObject *parent );

    /**
     * Destroys the transaction rollback job.
     */
    ~TransactionRollbackJob();

  protected:
    virtual void doStart();

  private:
    Q_DECLARE_PRIVATE( TransactionRollbackJob )
};


/**
 * @short Job that commits a session-global transaction.
 *
 * This job commits all changes of this transaction.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT TransactionCommitJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a new transaction commit job.
     * The parent must be the same parent as for the TransactionBeginJob.
     *
     * @param parent The parent job or Session, must not be 0.
     */
    explicit TransactionCommitJob( QObject *parent );

    /**
     * Destroys the transaction commit job.
     */
    ~TransactionCommitJob();

  protected:
    virtual void doStart();

  private:
    Q_DECLARE_PRIVATE( TransactionCommitJob )
};

}

#endif
