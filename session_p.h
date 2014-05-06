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

#include "akonadiprivate_export.h"
#include "session.h"
#include "imapparser_p.h"
#include "item.h"
#include "servermanager.h"

#include <QtNetwork/QLocalSocket>

#include <QtCore/QQueue>
#include <QtCore/QThreadStorage>
#include <QFile>

class QIODevice;

namespace Akonadi {

/**
 * @internal
 */
class AKONADI_TESTS_EXPORT SessionPrivate
{
public:
    explicit SessionPrivate(Session *parent);

    virtual ~SessionPrivate()
    {
        delete parser;
    }

    virtual void init(const QByteArray &sessionId);

    void startNext();
    /// Disconnects a previously existing connection and tries to reconnect
    void forceReconnect();
    /// Attemps to establish a connections to the Akonadi server.
    virtual void reconnect();
    void serverStateChanged(ServerManager::State);
    void socketDisconnected();
    void socketError(QLocalSocket::LocalSocketError error);
    void socketError(QAbstractSocket::SocketError error);
    void dataReceived();
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
    int nextTag();

    /**
      Sends the given raw data.
    */
    void writeData(const QByteArray &data);

    /**
     * Propagate item revision changes to following jobs.
     */
    void itemRevisionChanged(Akonadi::Item::Id itemId, int oldRevision, int newRevision);

    static int minimumProtocolVersion()
    {
        return 39;
    }

    /**
     * Default location for akonadiconnectionrc
     */
    static QString connectionFile();

    Session *mParent;
    QByteArray sessionId;
    QIODevice *socket;
    bool connected;
    int theNextTag;
    int protocolVersion;

    // job management
    QQueue<Job *> queue;
    QQueue<Job *> pipeline;
    Job *currentJob;
    bool jobRunning;

    // parser stuff
    ImapParser *parser;

    QFile *logFile;
};

}

#endif
