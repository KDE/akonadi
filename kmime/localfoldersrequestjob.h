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
  TODO

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_EXPORT LocalFoldersRequestJob : public TransactionSequence
{
  Q_OBJECT

  friend class LocalFoldersRequestJobPrivate;

  public:
    explicit LocalFoldersRequestJob( QObject *parent = 0 );
    ~LocalFoldersRequestJob();

    void requestDefaultFolder( int type );
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
