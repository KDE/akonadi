/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_JOB_P_H
#define AKONADI_JOB_P_H

#include "session.h"
#include "item.h"

namespace Akonadi {

/**
 * @internal
 */
class JobPrivate
{
public:
    explicit JobPrivate(Job *parent)
        : q_ptr(parent)
        , mCurrentSubJob(0)
        , mSession(0)
        , mWriteFinished(false)
        , mStarted(false)
    {
    }

    virtual ~JobPrivate()
    {
    }

    void init(QObject *parent);

    void handleResponse(const QByteArray &tag, const QByteArray &data);
    void startQueued();
    void lostConnection();
    void slotSubJobAboutToStart(Akonadi::Job *job);
    void startNext();
    void signalCreationToJobTracker();
    void signalStartedToJobTracker();
    void delayedEmitResult();
    /*
      Returns a string to display in akonadi console's job tracker. E.g. item ID.
     */
    virtual QString jobDebuggingString() const {
        return QString();
    }
    /**
      Returns a new unique command tag for communication with the backend.
    */
    QByteArray newTag();

    /**
      Return the tag used for the request.
    */
    QByteArray tag() const;

    /**
      Sends raw data to the backend.
    */
    void writeData(const QByteArray &data);

    /**
     * Notify following jobs about item revision changes.
     * This is used to avoid phantom conflicts between pipelined modify jobs on the same item.
     * @param itemID the id of the item which has changed
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

    int protocolVersion() const;

    Job *q_ptr;
    Q_DECLARE_PUBLIC(Job)

    Job *mParentJob;
    Job *mCurrentSubJob;
    QByteArray mTag;
    Session *mSession;
    bool mWriteFinished;
    bool mStarted;
};

}

#endif
