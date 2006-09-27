/*
    This file is part of libakonadi.

    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
                  2006 Marc Mutz <mutz@kde.org>

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

#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>

#include <kdepim_export.h>

namespace Akonadi {

/**
  This class encapsulates a reference to a pim object in
  the pim storage service.
 */
class AKONADI_EXPORT DataReference
{
  public:
    typedef QList<DataReference> List;

    /**
      Creates an empty DataReference.
     */
    DataReference();

    /**
      Creates a new DataReference with the given persistanceID and externalUrl.
     */
    DataReference( uint persistanceID, const QString &externalUrl );

    /**
      Destroys the DataReference.
     */
    ~DataReference();

    /**
      Returns the persistance id of the DataReference or an empty string if no
      id is set.
     */
    uint persistanceID() const;

    /**
      Returns the external url of the DataReference or an empty string if no
      url is set.
     */
    QUrl externalUrl() const;

    /**
      Returns true if this is a empty reference, ie. one created with
      DataReference().
    */
    bool isNull() const;

    /**
      Returns true if two references are equal.
    */
    bool operator==( const DataReference &other ) const;

    /**
      Returns true if two references are not equal.
    */
    bool operator!=( const DataReference &other ) const;

    /**
      Compares two references.
    */
    bool operator<( const DataReference &other ) const;

  private:
    uint mPersistanceID;
    QString mExternalUrl;
    bool mIsNull;

    class DataReferencePrivate;
    DataReferencePrivate* d;
};

/**
  This class encapsulates a request to the pim storage service,
  the code looks like

  \code
    Akonadi::Job *job = new Akonadi::SomeJob( some parameter );
    connect( job, SIGNAL( done( Akonadi::Job* ) ),
             this, SLOT( slotResult( Akonadi::Job* ) ) );
    job->start();
  \endcode

  And the slotResult is usually at least:

  \code
    if ( job->error() )
      // handle error...
  \endcode

  With the synchronous interface the code looks like

  \code
    Akonadi::SomeJob job( some parameter );
    if ( !job.exec() ) {
      qDebug( "Error: %s", qPrintable( job.errorString() ) );
    } else {
      // do something
    }
  \endcode

  Subclasses must reimplement @see doStart().
 */
class AKONADI_EXPORT Job : public QObject
{
  Q_OBJECT

  public:

    typedef QList<Job*> List;

    /**
      Error codes that can be emitted by this class,
      subclasses can provide additional codes.
     */
    enum Error
    {
      None,
      ConnectionFailed,
      UserCanceled,
      Unknown
    };

    /**
      Creates a new job.
      If the parent object is a Job object, the new job will use the same socket to
      communicate with the backend as the parent job.
      @param parent The parent object or parent job.
     */
    Job( QObject *parent = 0 );

    /**
      Destroys the job.
     */
    virtual ~Job();

    /**
      Executes the job synchronous.
     */
    bool exec();

    /**
      Starts the job asynchronous. When the job finished successful,
      @see done() is emitted.
     */
    void start();

    /**
      Abort this job.

      The @see done() signal is emitted nonetheless, but @see error()
      returns a special error code.
     */
    virtual void kill();

    /**
      Returns the error code, if there has been an error, 0 otherwise.
     */
    int error() const;

    /**
      Return error message, if there has been an error, QString() otherwise.
    */
    QString errorMessage() const;

    /**
      Returns the error string, if there has been an error, an empty
      string otherwise.
     */
    virtual QString errorText() const;

  Q_SIGNALS:
    /**
      This signal is emitted when the started job finished.

      @param job A pointer to the job which emitted the signal.
     */
    void done( Akonadi::Job *job );

    /**
      Progress signal showing the overall progress of the job.

      @param job The job that emitted this signal.
      @param percent The percentage.
     */
    void percent( Akonadi::Job *job, unsigned int percent );

    /**
      Emitted directly before the job will be started.
      @param job The started job.
    */
    void aboutToStart( Akonadi::Job *job );

  protected:
    /**
      Subclasses have to use this method to set an error code. They can
      optionally set a describing error message.
     */
    void setError( int code, const QString &msg = QString() );

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
    virtual void addSubJob( Job* job );

  private:
    void handleResponse( const QByteArray &tag, const QByteArray &data );

  private Q_SLOTS:
    void slotDisconnected();
    void slotDataReceived();
    void slotSocketError();
    void slotSubJobAboutToStart( Akonadi::Job* job );
    void slotSubJobDone( Akonadi::Job* job );

  private:
    class JobPrivate;
    JobPrivate *d;
};

}

AKONADI_EXPORT uint qHash( const Akonadi::DataReference& ref );

Q_DECLARE_METATYPE(Akonadi::DataReference)

#endif
