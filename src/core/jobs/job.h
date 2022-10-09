/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>
                  2006 - 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <KCompositeJob>

#include <memory>

class QString;

namespace Akonadi
{
namespace Protocol
{
class Command;
using CommandPtr = QSharedPointer<Command>;
}

class JobPrivate;
class Session;
class SessionPrivate;

/**
 * @short Base class for all actions in the Akonadi storage.
 *
 * This class encapsulates a request to the pim storage service,
 * the code looks like
 *
 * @code
 *
 *  Akonadi::Job *job = new Akonadi::SomeJob( some parameter );
 *  connect( job, SIGNAL(result(KJob*)),
 *           this, SLOT(slotResult(KJob*)) );
 *
 * @endcode
 *
 * The job is queued for execution as soon as the event loop is entered
 * again.
 *
 * And the slotResult is usually at least:
 *
 * @code
 *
 *  if ( job->error() ) {
 *    // handle error...
 *  }
 *
 * @endcode
 *
 * With the synchronous interface the code looks like
 *
 * @code
 *   Akonadi::SomeJob *job = new Akonadi::SomeJob( some parameter );
 *   if ( !job->exec() ) {
 *     qDebug() << "Error:" << job->errorString();
 *   } else {
 *     // do something
 *   }
 * @endcode
 *
 * @warning Using the synchronous method is error prone, use this only
 *          if the asynchronous access is not possible. See the documentation of
 *          KJob::exec() for more details.
 *
 * Subclasses must reimplement doStart().
 *
 * @note KJob-derived objects delete itself, it is thus not possible
 * to create job objects on the stack!
 *
 * @author Volker Krause <vkrause@kde.org>, Tobias Koenig <tokoe@kde.org>, Marc Mutz <mutz@kde.org>
 */
class AKONADICORE_EXPORT Job : public KCompositeJob
{
    Q_OBJECT

    friend class Session;
    friend class SessionPrivate;

public:
    /**
     * Describes a list of jobs.
     */
    using List = QList<Job *>;

    /**
     * Describes the error codes that can be emitted by this class.
     * Subclasses can provide additional codes, starting from UserError
     * onwards
     */
    enum Error {
        ConnectionFailed = UserDefinedError, ///< The connection to the Akonadi server failed.
        ProtocolVersionMismatch, ///< The server protocol version is too old or too new.
        UserCanceled, ///< The user canceled this job.
        Unknown, ///< Unknown error.
        UserError = UserDefinedError + 42 ///< Starting point for error codes defined by sub-classes.
    };

    /**
     * Creates a new job.
     *
     * If the parent object is a Job object, the new job will be a subjob of @p parent.
     * If the parent object is a Session object, it will be used for server communication
     * instead of the default session.
     *
     * @param parent The parent object, job or session.
     */
    explicit Job(QObject *parent = nullptr);

    /**
     * Destroys the job.
     */
    ~Job() override;

    /**
     * Jobs are started automatically once entering the event loop again, no need
     * to explicitly call this.
     */
    void start() override;

    /**
     * Returns the error string, if there has been an error, an empty
     * string otherwise.
     */
    Q_REQUIRED_RESULT QString errorString() const final;

Q_SIGNALS:
    /**
     * This signal is emitted directly before the job will be started.
     *
     * @param job The started job.
     */
    void aboutToStart(Akonadi::Job *job);

    /**
     * This signal is emitted if the job has finished all write operations, ie.
     * if this signal is emitted, the job guarantees to not call writeData() again.
     * Do not emit this signal directly, call emitWriteFinished() instead.
     *
     * @param job This job.
     * @see emitWriteFinished()
     */
    void writeFinished(Akonadi::Job *job);

protected:
    /**
     * This method must be reimplemented in the concrete jobs. It will be called
     * after the job has been started and a connection to the Akonadi backend has
     * been established.
     */
    virtual void doStart() = 0;

    /**
     * This method should be reimplemented in the concrete jobs in case you want
     * to handle incoming data. It will be called on received data from the backend.
     * The default implementation does nothing.
     *
     * @param tag The tag of the corresponding command, empty if this is an untagged response.
     * @param response The received response
     *
     * @return Implementations should return true if the last response was processed and
     * the job can emit result. Return false if more responses from server are expected.
     */
    virtual bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response);

    /**
     * Adds the given job as a subjob to this job. This method is automatically called
     * if you construct a job using another job as parent object.
     * The base implementation does the necessary setup to share the network connection
     * with the backend.
     *
     * @param job The new subjob.
     */
    bool addSubjob(KJob *job) override;

    /**
     * Removes the given subjob of this job.
     *
     * @param job The subjob to remove.
     */
    bool removeSubjob(KJob *job) override;

    /**
     * Kills the execution of the job.
     */
    bool doKill() override;

    /**
     * Call this method to indicate that this job will not call writeData() again.
     * @see writeFinished()
     */
    void emitWriteFinished();

protected Q_SLOTS:
    void slotResult(KJob *job) override;

protected:
    /// @cond PRIVATE
    Job(JobPrivate *dd, QObject *parent);
    std::unique_ptr<JobPrivate> const d_ptr;
    /// @endcond

private:
    Q_DECLARE_PRIVATE(Job)

    /// @cond PRIVATE
    Q_PRIVATE_SLOT(d_func(), void startNext())
    Q_PRIVATE_SLOT(d_func(), void signalCreationToJobTracker())
    Q_PRIVATE_SLOT(d_func(), void signalStartedToJobTracker())
    Q_PRIVATE_SLOT(d_func(), void delayedEmitResult())
    /// @endcond
};

}
