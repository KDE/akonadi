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

#ifndef AKONADI_LOCALFOLDERSHELPERJOBS_P_H
#define AKONADI_LOCALFOLDERSHELPERJOBS_P_H

#include "akonadi-kmime_export.h"

#include <akonadi/collection.h>
#include <akonadi/transactionsequence.h>

namespace Akonadi {

// ===================== ResourceScanJob ============================

/**
  TODO 

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_TEST_EXPORT ResourceScanJob : public TransactionSequence
{
  Q_OBJECT

  public:
    explicit ResourceScanJob( const QString &resourceId, QObject *parent = 0 );
    ~ResourceScanJob();

    QString resourceId() const;
    void setResourceId( const QString &resourceId );

    Akonadi::Collection rootResourceCollection() const;

    Akonadi::Collection::List localFolders() const;

  protected:
    /* reimpl */
    virtual void doStart();

  private:
    class Private;
    friend class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void fetchResult( KJob* ) )
};


// ===================== DefaultResourceJob ============================

/**
  TODO 

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_TEST_EXPORT DefaultResourceJob : public ResourceScanJob
{
  Q_OBJECT

  public:
    explicit DefaultResourceJob( QObject *parent = 0 );
    ~DefaultResourceJob();

  protected:
    /* reimpl */
    virtual void doStart();
    /* reimpl */
    virtual void slotResult( KJob *job );

  private:
    class Private;
    friend class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void resourceCreateResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void resourceSyncResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionFetchResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionModifyResult( KJob* ) )
};


// ===================== GetLockJob ============================

/**
  TODO 

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_TEST_EXPORT GetLockJob : public KJob
{
  Q_OBJECT

  public:
    explicit GetLockJob( QObject *parent = 0 );
    ~GetLockJob();

    virtual void start();

  private:
    class Private;
    friend class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void doStart() )
    Q_PRIVATE_SLOT( d, void serviceOwnerChanged( QString, QString, QString ) )
    Q_PRIVATE_SLOT( d, void timeout() )
};


// ===================== helper functions ============================

QString AKONADI_KMIME_TEST_EXPORT nameForType( int type );
QString displayNameForType( int type );
QString iconNameForType( int type );
void setCollectionAttributes( Akonadi::Collection &col, int type );
bool AKONADI_KMIME_TEST_EXPORT releaseLock();

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERSHELPERJOBS_P_H
