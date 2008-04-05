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

#include "session.h"
#include "imapparser_p.h"

#include <QtCore/QQueue>
#include <QtCore/QSettings>
#include <QtCore/QThreadStorage>

class QLocalSocket;

namespace Akonadi {

class SessionPrivate
{
  public:
    SessionPrivate( Session *parent )
      : mParent( parent ), mConnectionSettings( 0 )
    {
      parser = new ImapParser();
    }

    ~SessionPrivate()
    {
      delete parser;
      delete mConnectionSettings;
    }

    void startNext();
    void reconnect();
    void socketError();
    void dataReceived();
    void doStartNext();
    void jobDone( KJob* job );
    void jobWriteFinished( Akonadi::Job* job );

    bool canPipelineNext();

    /**
     * Creates a new default session for this thread with
     * the given @p sessionId. The session can be accessed
     * later by defaultSession().
     *
     * You only need to call this method if you want that the
     * default session has a special custom id, otherwise a random unique
     * id is used automatically.
     */
    static void createDefaultSession( const QByteArray &sessionId );

    /**
      Associates the given Job object with this session.
    */
    void addJob( Job* job );

    /**
      Returns the next IMAP tag.
    */
    int nextTag();

    /**
      Sends the given raw data.
    */
    void writeData( const QByteArray &data );

    Session *mParent;
    QByteArray sessionId;
    QSettings *mConnectionSettings;
    QLocalSocket* socket;
    bool connected;
    int theNextTag;

    // job management
    QQueue<Job*> queue;
    QQueue<Job*> pipeline;
    Job* currentJob;
    bool jobRunning;

    // parser stuff
    ImapParser *parser;
};

}

#endif
