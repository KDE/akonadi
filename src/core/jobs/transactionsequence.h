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

#ifndef AKONADI_TRANSACTIONSEQUENCE_H
#define AKONADI_TRANSACTIONSEQUENCE_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi {

class TransactionSequencePrivate;

/**
 * @short Base class for jobs that need to run a sequence of sub-jobs in a transaction.
 *
 * As soon as the first subjob is added, the transaction is started.
 * As soon as the last subjob has successfully finished, the transaction is committed.
 * If any subjob fails, the transaction is rolled back.
 *
 * Alternatively, a TransactionSequence object can be used as a parent object
 * for a set of jobs to achieve the same behaviour without subclassing.
 *
 * Example:
 *
 * @code
 *
 * // Delete a couple of items inside a transaction
 * Akonadi::TransactionSequence *transaction = new Akonadi::TransactionSequence;
 * connect( transaction, SIGNAL(result(KJob*)), SLOT(transactionFinished(KJob*)) );
 *
 * const Akonadi::Item::List items = ...
 *
 * foreach ( const Akonadi::Item &item, items ) {
 *   new Akonadi::ItemDeleteJob( item, transaction );
 * }
 *
 * ...
 *
 * MyClass::transactionFinished( KJob *job )
 * {
 *   if ( job->error() )
 *     qDebug() << "Error occurred";
 *   else
 *     qDebug() << "Items deleted successfully";
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT TransactionSequence : public Job
{
    Q_OBJECT
public:
    /**
     * Creates a new transaction sequence.
     *
     * @param parent The parent object.
     */
    explicit TransactionSequence(QObject *parent = 0);

    /**
     * Destroys the transaction sequence.
     */
    ~TransactionSequence();

    /**
     * Commits the transaction as soon as all pending sub-jobs finished successfully.
     */
    void commit();

    /**
     * Rolls back the current transaction as soon as possible.
     * You only need to call this method when you want to roll back due to external
     * reasons (e.g. user cancellation), the transaction is automatically rolled back
     * if one of its subjobs fails.
     * @since 4.5
     */
    void rollback();

    /**
     * Sets which job of the sequence might fail without rolling back the
     * complete transaction.
     * @param job a job to ignore errors from
     * @since 4.5
     */
    void setIgnoreJobFailure(KJob *job);

    /**
     * Disable automatic committing.
     * Use this when you want to add jobs to this sequence after execution
     * has been started, usually that is outside of the constructor or the
     * method that creates this transaction sequence.
     * @note Calling this method after execution of this job has been started
     * has no effect.
     * @param enable @c true to enable autocommitting (default), @c false to disable it
     * @since 4.5
     */
    void setAutomaticCommittingEnabled(bool enable);

protected:
    bool addSubjob(KJob *job) Q_DECL_OVERRIDE;
    void doStart() Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void slotResult(KJob *job) Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(TransactionSequence)

    //@cond PRIVATE
    Q_PRIVATE_SLOT(d_func(), void commitResult(KJob *))
    Q_PRIVATE_SLOT(d_func(), void rollbackResult(KJob *))
    //@endcond
};

}

#endif
