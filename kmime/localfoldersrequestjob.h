/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef AKONADI_LOCALFOLDERSREQUESTJOB_H
#define AKONADI_LOCALFOLDERSREQUESTJOB_H

#include "akonadi-kmime_export.h"

#include <akonadi/collection.h>
#include <akonadi/transactionsequence.h>

namespace Akonadi {

class LocalFoldersRequestJobPrivate;

/**
  @short A job to request LocalFolders.

  Use this job to request the LocalFolders you need. You can request both
  default LocalFolders and LocalFolders in a given resource. The default
  LocalFolders resource is created when the first default LocalFolder is
  requested, but if a LocalFolder in a custom resource is requested, this
  job expects that resource to exist already.

  If the folders you requested already exist, this job simply succeeds.
  Otherwise, it creates the required collections and registers them with
  LocalFolders.

  Example of usage:

  In your class constructor:
  @code
  LocalFoldersRequestJob *rjob = new LocalFoldersRequestJob( this );
  rjob->requestDefaultFolder( LocalFolders::Outbox );
  connect( rjob, SIGNAL(result(KJob*)), this, SLOT(localFoldersRequestResult(KJob*)) );
  rjob->start();
  @endcode

  And in your localFoldersRequestResult( KJob *job ) slot:
  @code
  if( job->error() ) { ... }
  Collection col = LocalFolders::self()->defaultFolder( LocalFolders::Outbox );
  Q_ASSERT( col.isValid() );
  @endcode

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_EXPORT LocalFoldersRequestJob : public TransactionSequence
{
  Q_OBJECT

  friend class LocalFoldersRequestJobPrivate;

  public:
    /**
      Creates a new LocalFoldersRequestJob.
    */
    explicit LocalFoldersRequestJob( QObject *parent = 0 );

    /**
      Destroys this LocalFoldersRequestJob.
    */
    ~LocalFoldersRequestJob();

    /**
      Requests a LocalFolder of type @p type in the default resource.
      @param type The type of LocalFolder requested.
    */
    void requestDefaultFolder( int type );

    /**
      Requests a LocalFolder of type @p type in the given resource.
      @param type The type of LocalFolder requested.
      @param resourceId The resource ID of the LocalFolder requested.
    */
    void requestFolder( int type, const QString &resourceId );

  protected:
    /* reimpl */
    virtual void doStart();
    /* reimpl */
    virtual void slotResult( KJob *job );

  private:
    LocalFoldersRequestJobPrivate *const d;

    Q_PRIVATE_SLOT( d, void lockResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void releaseLock() )
    Q_PRIVATE_SLOT( d, void resourceScanResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionCreateResult( KJob* ) )

};

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERSREQUESTJOB_H
