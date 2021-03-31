/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "item.h"
#include "session.h"

namespace Akonadi
{
namespace Protocol
{
class Command;
}

/**
 * @internal
 */
class JobPrivate
{
public:
    explicit JobPrivate(Job *parent)
        : q_ptr(parent)
    {
    }

    virtual ~JobPrivate() = default;

    void init(QObject *parent);

    void handleResponse(qint64 tag, const Protocol::CommandPtr &response);
    void startQueued();
    void lostConnection();
    void slotSubJobAboutToStart(Akonadi::Job *job);
    void startNext();
    void signalCreationToJobTracker();
    void signalStartedToJobTracker();
    void delayedEmitResult();
    void publishJob();
    /*
      Returns a string to display in akonadi console's job tracker. E.g. item ID.
     */
    virtual QString jobDebuggingString() const
    {
        return QString();
    }
    /**
      Returns a new unique command tag for communication with the backend.
    */
    Q_REQUIRED_RESULT qint64 newTag();

    /**
      Return the tag used for the request.
    */
    Q_REQUIRED_RESULT qint64 tag() const;

    /**
      Sends the @p command to the backend
     */
    void sendCommand(qint64 tag, const Protocol::CommandPtr &command);

    /**
     * Same as calling JobPrivate::sendCommand(newTag(), command)
     */
    void sendCommand(const Protocol::CommandPtr &command);

    /**
     * Notify following jobs about item revision changes.
     * This is used to avoid phantom conflicts between pipelined modify jobs on the same item.
     * @param itemId the id of the item which has changed
     * @param oldRevision the old item revision
     * @param newRevision the new item revision
     */
    void itemRevisionChanged(Akonadi::Item::Id itemId, int oldRevision, int newRevision);

    /**
     * Propagate item revision changes to this job and its sub-jobs.
     */
    void updateItemRevision(Akonadi::Item::Id itemId, int oldRevision, int newRevision);

    /**
     * Overwrite this if your job does operations with conflict detection and update
     * the item revisions if your items are affected. The default implementation does nothing.
     */
    virtual void doUpdateItemRevision(Akonadi::Item::Id, int oldRevision, int newRevision);

    /**
     * This method is called right before result() and finished() signals are emitted.
     * Overwrite this method in your job if you need to emit some signals or process
     * some data before the job finishes.
     *
     * Default implementation does nothing.
     */
    virtual void aboutToFinish();

    Q_REQUIRED_RESULT int protocolVersion() const;

    Job *q_ptr;
    Q_DECLARE_PUBLIC(Job)

    Job *mParentJob = nullptr;
    Job *mCurrentSubJob = nullptr;
    qint64 mTag = -1;
    Session *mSession = nullptr;
    bool mWriteFinished = false;
    bool mReadingFinished = false;
    bool mStarted = false;
    bool mFinishPending = false;

private:
    Q_DISABLE_COPY_MOVE(JobPrivate)
};

}

