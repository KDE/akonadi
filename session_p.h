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
#include "imapparser.h"
#include <QQueue>
#include <QtCore/QSettings>
#include <QThreadStorage>

#ifdef Q_OS_WIN
class QTcpSocket;
#else
class KLocalSocket;
#endif

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

    Session *mParent;
    QByteArray sessionId;
    QSettings *mConnectionSettings;
#ifdef Q_OS_WIN
    QTcpSocket* socket;
#else
    KLocalSocket* socket;
#endif
    bool connected;
    int nextTag;

    // job management
    QQueue<Job*> queue;
    Job* currentJob;
    bool jobRunning;

    // parser stuff
    ImapParser *parser;
};

}

#endif
