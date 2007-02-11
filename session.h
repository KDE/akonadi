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

#ifndef AKONADI_SESSION_H
#define AKONADI_SESSION_H

#include <QtCore/QObject>

#include <kdepim_export.h>

class KJob;

namespace Akonadi {

class Job;

/**
  A communication session with the Akonadi storage. Every Job object has to
  be associated with a Session. The session is responsible of scheduling its
  jobs. For now only a simple serial execution is impleneted (the IMAP-like
  protocol to communicate with the storage backend is capable of parallel
  execution on a single session though).
*/
class AKONADI_EXPORT Session : public QObject
{
  Q_OBJECT

  friend class Job;

  public:
    /**
      Open a new session to the Akonadi storage.
      @param sessionId The identifier for this session, will be a
      random vaule if empty.
      @param parent The parent object.

      @see defaultSession()
    */
    Session( const QByteArray &sessionId = QByteArray(), QObject *parent = 0 );

    /**
      Closes this session.
    */
    ~Session();

    /**
      Returns the session identifier.
    */
    QByteArray sessionId() const;

    /**
      Returns the default session for this thread.
    */
    static Session* defaultSession();

    /**
      Stops all jobs queued for execution.
    */
    void clear();

  protected:
    /**
      Associates the given Job object with this session.
    */
    void addJob( Job* job );

    /**
      Sends the given raw data.
    */
    void writeData( const QByteArray &data );

    /**
      Returns the next IMAP tag.
    */
    int nextTag();

  private:
    void startNext();

  private slots:
    void reconnect();
    void disconnected();
    void socketError();
    void dataReceived();
    void doStartNext();
    void jobDone( KJob* job );

  private:
    class Private;
    Private* const d;
};

}


#endif
