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

#ifndef PIM_JOB_H
#define PIM_JOB_H

#include <QObject>
#include <QString>
#include <QUrl>

namespace PIM {

/**
  This class encapsulates a reference to a pim object in
  the pim storage service.
 */
class DataReference
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
    DataReference( const QString &persistanceID, const QString &externalUrl );

    /**
      Destroys the DataReference.
     */
    ~DataReference();

    /**
      Returns the persistance id of the DataReference or an empty string if no
      id is set.
     */
    QString persistanceID() const;

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
      Comapares to references.
    */
    bool operator<( const DataReference &other ) const;

  private:
    QString mPersistanceID;
    QString mExternalUrl;

    class DataReferencePrivate;
    DataReferencePrivate *d;
};

/**
  This class encapsulates a request to the pim storage service,
  the code looks like

  \code
    PIM::Job *job = new PIM::SomeJob( some parameter );
    connect( job, SIGNAL( done( PIM::Job* ) ),
             this, SLOT( slotResult( PIM::Job* ) ) );
    job->start();
  \endcode

  And the slotResult is usually at least:

  \code
    if ( job->error() )
      // handle error...
  \endcode

  With the synchronous interface the code looks like

  \code
    PIM::SomeJob job( some parameter );
    if ( !job.exec() ) {
      qDebug( "Error: %s", qPrintable( job.errorString() ) );
    } else {
      // do something
    }
  \endcode

  Subclasses must reimplement @see doStart().
 */
class Job : public QObject
{
  Q_OBJECT

  public:

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
      Starts the job asynchronous. When the job finished successfull,
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
      Returns the error string, if there has been an error, an empty
      string otherwise.
     */
    virtual QString errorText() const;

  signals:
    /**
      This signal is emitted when the started job finished.

      @param job A pointer to the job which emitted the signal.
     */
    void done( PIM::Job *job );

    /**
      Progress signal showing the overall progress of the job.

      @param job The job that emitted this signal.
      @param percent The percentage.
     */
    void percent( PIM::Job *job, unsigned int percent );

  protected:
    /**
      Subclasses have to use this method to set an error code.
     */
    void setError( int );

  private:
    /**
      This method must be reimplemented in the concrete jobs.

      Implementations must not emit the done() signal directly in this method,
      since this will cause a deadlock in exec(). Use a singleshot timer
      instead.
     */
    virtual void doStart() = 0;

    class JobPrivate;
    JobPrivate *d;
};

}

uint qHash( const PIM::DataReference& ref );

#endif
