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
  @internal
  Helper job for LocalFoldersRequestJob.

  A Job that fetches all the collections of a resource, and returns only
  those that have a LocalFolderAttribute.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_TEST_EXPORT ResourceScanJob : public TransactionSequence
{
  Q_OBJECT

  public:
    /**
      Creates a new ResourceScanJob.
    */
    explicit ResourceScanJob( const QString &resourceId, QObject *parent = 0 );

    /**
      Destroys this ResourceScanJob.
    */
    ~ResourceScanJob();

    /**
      Returns the resource ID of the resource being scanned.
    */
    QString resourceId() const;
    
    /**
      Sets the resource ID of the resource to scan.
    */
    void setResourceId( const QString &resourceId );

    /**
      Returns the root collection of the resource being scanned.
      This function relies on there being a single top-level collection owned
      by this resource.
    */
    Akonadi::Collection rootResourceCollection() const;

    /**
      Returns all the collections of this resource which have a
      LocalFolderAttribute. These might include the root resource collection.
    */
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
  @internal
  Helper job for LocalFoldersRequestJob.

  A custom ResourceScanJob for the default local folders resource. This is a
  maildir resource stored in ~/.local/share/local-mail.

  This job does two things that a regular ResourceScanJob does not do:
  1) It creates and syncs the resource if it is missing. The resource ID is
     stored in a config file named localfoldersrc.
  2) If the resource had to be recreated, but the folders existed on disk
     before that, it recovers the folders based on name. For instance, it will
     give a folder named outbox a LocalFolderAttribute of type Outbox.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_TEST_EXPORT DefaultResourceJob : public ResourceScanJob
{
  Q_OBJECT

  public:
    /**
      Creates a new DefaultResourceJob.
    */
    explicit DefaultResourceJob( QObject *parent = 0 );

    /**
      Destroys the DefaultResourceJob.
    */
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
  @internal
  Helper job for LocalFoldersRequestJob.

  If LocalFoldersRequestJob needs to create a collection, it sets a lock so
  that other instances do not interfere. This lock is an
  org.kde.pim.LocalFolders name registered on D-Bus. This job is used to get
  that lock.
  This job will give the lock immediately if possible, or wait up to three
  seconds for the lock to be released.  If the lock is not released during
  that time, this job fails. (This is based on the assumption that
  LocalFoldersRequestJob operations should not take long.)

  Use the releaseLock() function to release the lock.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_TEST_EXPORT GetLockJob : public KJob
{
  Q_OBJECT

  public:
    /**
      Creates a new GetLockJob.
    */
    explicit GetLockJob( QObject *parent = 0 );

    /**
      Destroys the GetLockJob.
    */
    ~GetLockJob();

    /* reimpl */
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

/**
  Returns the short English name associated with a LocalFolder type (for
  instance 'outbox' for Outbox). These names are used as collection names.
*/
QString AKONADI_KMIME_TEST_EXPORT nameForType( int type );

/**
  Returns the pretty i18n'ed name of a LocalFolder type. These names are used
  in the EntityDisplayAttribute of LocalFolders collections.
*/
QString displayNameForType( int type );

/**
  Returns the icon name for a given LocalFolder type.
*/
QString iconNameForType( int type );

/**
  Sets on @p col the required attributes of LocalFolder type @p type.
  These are a LocalFolderAttribute and an EntityDisplayAttribute.
*/
void setCollectionAttributes( Akonadi::Collection &col, int type );

/**
  Releases the LocalFoldersRequestJob lock that was obtained through
  GetLockJob.
  @return Whether the lock was released successfully.
*/
bool AKONADI_KMIME_TEST_EXPORT releaseLock();

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERSHELPERJOBS_P_H
