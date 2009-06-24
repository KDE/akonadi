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

#ifndef AKONADI_LOCALFOLDERSBUILDJOB_P_H
#define AKONADI_LOCALFOLDERSBUILDJOB_P_H

#include <akonadi/collection.h>
#include <akonadi/transactionsequence.h>

namespace Akonadi {

/**
  A job building and fetching the local folder structure for LocalFolders.

  Whether this job is allowed to create stuff or not is determined by whether
  the LocalFolders who created it is the main instance of LocalFolders or not.
  (This is done to avoid race conditions.)

  Logic if main instance:
  1) if loading the resource from config fails, create the resource.
  2) if after fetching, some collections need to be created / modified, do that.
  3) if creating / modification succeeded -> success.
     else -> failure.

  Logic if not main instance:
  1) if loading the resource from config fails -> failure.
  2) if after fetching, some collections need to be created / modified -> failure.
  2) if all collections were fetched ok -> success.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class LocalFoldersBuildJob : public TransactionSequence
{
  Q_OBJECT

  public:
    /**
      @param canBuild Whether this job is allowed to build the folder
                      structure (as opposed to only fetching it).
    */
    LocalFoldersBuildJob( bool canBuild, QObject *parent = 0 );
    ~LocalFoldersBuildJob();

    const Collection::List &defaultFolders() const;
    //const QSet<Collection> &customFolders() const;

  protected:
    /* reimpl */
    virtual void doStart();

  private:
    class Private;
    friend class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void resourceCreateResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void resourceSyncResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionFetchResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionCreateResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionModifyResult( KJob* ) )

};

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERSBUILDJOB_P_H
