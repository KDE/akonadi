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

#include "akonadi_export.h"
#include <QtCore/QObject>

class KJob;

namespace Akonadi {

class Job;
class SessionPrivate;

/**
 * @short A communication session with the Akonadi storage.
 *
 * Every Job object has to be associated with a Session.
 * The session is responsible of scheduling its jobs.
 * For now only a simple serial execution is impleneted (the IMAP-like
 * protocol to communicate with the storage backend is capable of parallel
 * execution on a single session though).
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * Session *session = new Session( "mySession" );
 *
 * CollectionFetchJob *job = new CollectionFetchJob( Collection::root(),
 *                                                   CollectionFetchJob::Recursive,
 *                                                   session );
 *
 * connect( job, SIGNAL( result( KJob* ) ), this, SLOT( slotResult( KJob* ) ) );
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT Session : public QObject
{
  Q_OBJECT

  friend class Job;
  friend class JobPrivate;
  friend class SessionPrivate;

  public:
    /**
     * Creates a new session.
     *
     * @param sessionId The identifier for this session, will be a
     *                  random value if empty.
     * @param parent The parent object.
     *
     * @see defaultSession()
     */
    explicit Session( const QByteArray &sessionId = QByteArray(), QObject *parent = 0 );

    /**
     * Destroys the session.
     */
    ~Session();

    /**
     * Returns the session identifier.
     */
    QByteArray sessionId() const;

    /**
     * Returns the default session for this thread.
     */
    static Session* defaultSession();

    /**
     * Stops all jobs queued for execution.
     */
    void clear();

  protected:
    /**
     * Creates a new session with shared private object.
     *
     * @param d The private object.
     * @param sessionId The identifier for this session, will be a
     *                  random value if empty.
     * @param parent The parent object.
     *
     * @note This constructor is needed for unit testing only.
     */
    explicit Session( SessionPrivate *d, const QByteArray &sessionId = QByteArray(), QObject *parent = 0 );

  private:
    //@cond PRIVATE
    SessionPrivate* const d;

    Q_PRIVATE_SLOT( d, void reconnect() )
    Q_PRIVATE_SLOT( d, void socketError(QLocalSocket::LocalSocketError) )
    Q_PRIVATE_SLOT( d, void socketDisconnected() )
    Q_PRIVATE_SLOT( d, void dataReceived() )
    Q_PRIVATE_SLOT( d, void doStartNext() )
    Q_PRIVATE_SLOT( d, void jobDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void jobWriteFinished( Akonadi::Job* ) )
    Q_PRIVATE_SLOT( d, void jobDestroyed( QObject* ) )
    //@endcond PRIVATE
};

}


#endif
