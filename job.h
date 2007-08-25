/*
    This file is part of libakonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>
                  2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_JOB_H
#define AKONADI_JOB_H

#include "libakonadi_export.h"

#include <QtCore/QObject>
#include <QtCore/QString>

#include <kcompositejob.h>
#include <libakonadi/datareference.h>


namespace Akonadi {

class Session;
class SessionPrivate;

/**
  This class encapsulates a request to the pim storage service,
  the code looks like

  \code
    Akonadi::Job *job = new Akonadi::SomeJob( some parameter );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( slotResult( KJob* ) ) );
    job->start();
  \endcode

  And the slotResult is usually at least:

  \code
    if ( job->error() )
      // handle error...
  \endcode

  With the synchronous interface the code looks like

  \code
    Akonadi::SomeJob *job = new Akonadi::SomeJob( some parameter );
    if ( !job->exec() ) {
      qDebug( "Error: %s", qPrintable( job->errorString() ) );
    } else {
      // do something
    }
  \endcode

  Subclasses must reimplement @see doStart().

  Note that KJob-derived objects delete itself, it is thus not possible
  to create job objects on the stack!
 */
class AKONADI_EXPORT Job : public KCompositeJob
{
  Q_OBJECT

  friend class Session;
  friend class SessionPrivate;

  public:

    typedef QList<Job*> List;

    /**
      Error codes that can be emitted by this class,
      subclasses can provide additional codes.
     */
    enum Error
    {
      ConnectionFailed = UserDefinedError,
      UserCanceled,
      Unknown
    };

    /**
      Creates a new job.
      If the parent object is a Job object, the new job will be a subjob of @p parent.
      If the parent object is a Session object, it will be used for server communication
      instead of the default session.
      @param parent The parent object, job or session.
     */
    Job( QObject *parent = 0 );

    /**
      Destroys the job.
     */
    virtual ~Job();

    /**
      Starts the job asynchronous. When the job finished successful,
      @see done() is emitted.
     */
    void start();

    /**
      Returns the error string, if there has been an error, an empty
      string otherwise.
     */
    virtual QString errorString() const;

    /**
      Returns the identifier of the session that executes this job.
    */
    QByteArray sessionId() const;

  Q_SIGNALS:
    /**
      Emitted directly before the job will be started.
      @param job The started job.
    */
    void aboutToStart( Akonadi::Job *job );

  protected:
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
    void writeData( const QByteArray &data );

    /**
      This method must be reimplemented in the concrete jobs. It will be called
      after the job has been started and a connection to the Akonadi backend has
      been established.
    */
    virtual void doStart() = 0;

    /**
      This method should be reimplemented in the concrete jobs in case you want
      to handle incoming data. It will be called on received data from the backend.
      The default implementation does nothing.
      @param tag The tag of the corresponding command, empty if this is an untagges response.
      @param data The received data.
    */
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

    /**
      Adds the given job as a subjob to this job. This method is automatically called
      if you construct a job using another job as parent object.
      The base implementation does the necessary setup to share the network connection
      with the backend.
      @param job The new subjob.
    */
    virtual bool addSubjob( KJob* job );

    virtual bool doKill();

  protected Q_SLOTS:
    virtual void slotResult( KJob* job );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void slotSubJobAboutToStart( Akonadi::Job* ) )
    Q_PRIVATE_SLOT( d, void startNext() )
};

}

#endif
