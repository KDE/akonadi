/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "commandbuffer_p.h"
#include "item.h"
#include "servermanager.h"
#include "session.h"

#include <QFile>
#include <QMetaObject>
#include <QQueue>
#include <QThreadStorage>

namespace Akonadi
{
class SessionThread;
class Connection;

namespace Protocol
{
class Command;
}

/*!
 * \brief Private implementation of Session.
 *
 * \internal
 *
 * \class Akonadi::SessionPrivate
 * \inheaderfile Akonadi/Session
 * \inmodule AkonadiCore
 */
class AKONADICORE_EXPORT SessionPrivate
{
public:
    explicit SessionPrivate(Session *parent);

    virtual ~SessionPrivate();

    virtual void init(const QByteArray &sessionId);

    SessionThread *sessionThread() const
    {
        return mSessionThread;
    }

    void enqueueCommand(qint64 tag, const Protocol::CommandPtr &cmd);

    void startNext();
    /// Disconnects a previously existing connection and tries to reconnect
    void forceReconnect();
    /// Attempts to establish a connections to the Akonadi server.
    virtual void reconnect();
    void serverStateChanged(ServerManager::State);
    void socketDisconnected();
    void socketError(const QString &error);
    void dataReceived();
    virtual bool handleCommands();
    void doStartNext();
    void startJob(Job *job);

    /*!
      \internal For testing purposes only. See FakeSesson.
      \a job the job to end
    */
    void endJob(Job *job);

    void jobDone(KJob *job);
    void jobWriteFinished(Akonadi::Job *job);
    void jobDestroyed(QObject *job);

    bool canPipelineNext();

    /*!
     * Creates a new default session for this thread with
     * the given \a sessionId. The session can be accessed
     * later by defaultSession().
     *
     * You only need to call this method if you want that the
     * default session has a special custom id, otherwise a random unique
     * id is used automatically.
     * \a sessionId the id of new default session
     */
    static void createDefaultSession(const QByteArray &sessionId);

    /*!
     * Sets the default session.
     * \internal Only for unit tests.
     */
    static void setDefaultSession(Session *session);

    /*!
      Associates the given Job object with this session.
    */
    virtual void addJob(Job *job);

    /*!
      Returns the next IMAP tag.
    */
    [[nodiscard]] qint64 nextTag();

    /*!
      Sends the given command to server
    */
    void sendCommand(qint64 tag, const Protocol::CommandPtr &command);

    /*!
     * Propagate item revision changes to following jobs.
     */
    void itemRevisionChanged(Akonadi::Item::Id itemId, int oldRevision, int newRevision);

    void clear(bool forceReconnect);

    void publishOtherJobs(Job *thanThisJob);

    Session *mParent = nullptr;
    SessionThread *mSessionThread = nullptr;
    Connection *connection = nullptr;
    QMetaObject::Connection connThreadCleanUp;
    QByteArray sessionId;
    bool connected;
    qint64 theNextTag;
    int protocolVersion;

    CommandBuffer mCommandBuffer;

    // job management
    QQueue<Job *> queue;
    QQueue<Job *> pipeline;
    Job *currentJob = nullptr;
    bool jobRunning;

    QFile *logFile = nullptr;
};

}
