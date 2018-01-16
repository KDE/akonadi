/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SESSION_P_H
#define AKONADI_SESSION_P_H

#include "akonadicore_export.h"
#include "session.h"
#include "item.h"
#include "servermanager.h"
#include "commandbuffer_p.h"


#include <QQueue>
#include <QThreadStorage>
#include <QMetaObject>
#include <QFile>

namespace Akonadi
{
class SessionThread;
class Connection;

namespace Protocol
{
class Command;
}

/**
 * @internal
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

    void startNext();
    /// Disconnects a previously existing connection and tries to reconnect
    void forceReconnect();
    /// Attemps to establish a connections to the Akonadi server.
    virtual void reconnect();
    void serverStateChanged(ServerManager::State);
    void socketDisconnected();
    void socketError(const QString &error);
    void dataReceived();
    virtual bool handleCommands();
    void doStartNext();
    void startJob(Job *job);

    /**
      @internal For testing purposes only. See FakeSesson.
      @param job the job to end
    */
    void endJob(Job *job);

    void jobDone(KJob *job);
    void jobWriteFinished(Akonadi::Job *job);
    void jobDestroyed(QObject *job);

    bool canPipelineNext();

    /**
     * Creates a new default session for this thread with
     * the given @p sessionId. The session can be accessed
     * later by defaultSession().
     *
     * You only need to call this method if you want that the
     * default session has a special custom id, otherwise a random unique
     * id is used automatically.
     * @param sessionId the id of new default session
     */
    static void createDefaultSession(const QByteArray &sessionId);

    /**
     * Sets the default session.
     * @internal Only for unit tests.
     */
    static void setDefaultSession(Session *session);

    /**
      Associates the given Job object with this session.
    */
    virtual void addJob(Job *job);

    /**
      Returns the next IMAP tag.
    */
    qint64 nextTag();

    /**
      Sends the given command to server
    */
    void sendCommand(qint64 tag, const Protocol::CommandPtr &command);

    /**
     * Propagate item revision changes to following jobs.
     */
    void itemRevisionChanged(Akonadi::Item::Id itemId, int oldRevision, int newRevision);

    /**
     * Default location for akonadiconnectionrc
     */
    static QString connectionFile();

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

#endif
