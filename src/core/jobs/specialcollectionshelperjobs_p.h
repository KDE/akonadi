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

#ifndef AKONADI_SPECIALCOLLECTIONSHELPERJOBS_P_H
#define AKONADI_SPECIALCOLLECTIONSHELPERJOBS_P_H

#include "akonadiprivate_export.h"
#include "collection.h"
#include "specialcollections.h"
#include "transactionsequence.h"

#include <QtCore/QVariant>

namespace Akonadi {

// ===================== ResourceScanJob ============================

/**
  @internal
  Helper job for SpecialCollectionsRequestJob.

  A Job that fetches all the collections of a resource, and returns only
  those that have a SpecialCollectionAttribute.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_TESTS_EXPORT ResourceScanJob : public Job
{
    Q_OBJECT

public:
    /**
      Creates a new ResourceScanJob.
    */
    explicit ResourceScanJob(const QString &resourceId, KCoreConfigSkeleton *settings, QObject *parent = 0);

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
    void setResourceId(const QString &resourceId);

    /**
      Returns the root collection of the resource being scanned.
      This function relies on there being a single top-level collection owned
      by this resource.
    */
    Akonadi::Collection rootResourceCollection() const;

    /**
      Returns all the collections of this resource which have a
      SpecialCollectionAttribute. These might include the root resource collection.
    */
    Akonadi::Collection::List specialCollections() const;

protected:
    /* reimpl */
    void doStart() Q_DECL_OVERRIDE;

private:
    class Private;
    friend class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void fetchResult(KJob *))
};

// ===================== DefaultResourceJob ============================

class DefaultResourceJobPrivate;

/**
  @internal
  Helper job for SpecialCollectionsRequestJob.

  A custom ResourceScanJob for the default local folders resource. This is a
  maildir resource stored in ~/.local/share/local-mail.

  This job does two things that a regular ResourceScanJob does not do:
  1) It creates and syncs the resource if it is missing. The resource ID is
     stored in a config file named specialcollectionsrc.
  2) If the resource had to be recreated, but the folders existed on disk
     before that, it recovers the folders based on name. For instance, it will
     give a folder named outbox a SpecialCollectionAttribute of type Outbox.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_TESTS_EXPORT DefaultResourceJob : public ResourceScanJob
{
    Q_OBJECT

public:
    /**
     * Creates a new DefaultResourceJob.
     */
    explicit DefaultResourceJob(KCoreConfigSkeleton *settings, QObject *parent = 0);

    /**
     * Destroys the DefaultResourceJob.
     */
    ~DefaultResourceJob();

    /**
     * Sets the @p type of the resource that shall be created if the requested
     * special collection does not exist yet.
     */
    void setDefaultResourceType(const QString &type);

    /**
     * Sets the configuration @p options that shall be applied to the new resource
     * that is created if the requested special collection does not exist yet.
     */
    void setDefaultResourceOptions(const QVariantMap &options);

    /**
     * Sets the list of well known special collection @p types.
     */
    void setTypes(const QList<QByteArray> &types);

    /**
     * Sets the @p map of special collection types to display names.
     */
    void setNameForTypeMap(const QMap<QByteArray, QString> &map);

    /**
     * Sets the @p map of special collection types to icon names.
     */
    void setIconForTypeMap(const QMap<QByteArray, QString> &map);

protected:
    /* reimpl */
    void doStart() Q_DECL_OVERRIDE;
    /* reimpl */
    void slotResult(KJob *job) Q_DECL_OVERRIDE;

private:
    friend class DefaultResourceJobPrivate;
    DefaultResourceJobPrivate *const d;

    Q_PRIVATE_SLOT(d, void resourceCreateResult(KJob *))
    Q_PRIVATE_SLOT(d, void resourceSyncResult(KJob *))
    Q_PRIVATE_SLOT(d, void collectionFetchResult(KJob *))
    Q_PRIVATE_SLOT(d, void collectionModifyResult(KJob *))
};

// ===================== GetLockJob ============================

/**
  @internal
  Helper job for SpecialCollectionsRequestJob.

  If SpecialCollectionsRequestJob needs to create a collection, it sets a lock so
  that other instances do not interfere. This lock is an
  org.kde.pim.SpecialCollections name registered on D-Bus. This job is used to get
  that lock.
  This job will give the lock immediately if possible, or wait up to three
  seconds for the lock to be released.  If the lock is not released during
  that time, this job fails. (This is based on the assumption that
  SpecialCollectionsRequestJob operations should not take long.)

  Use the releaseLock() function to release the lock.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_TESTS_EXPORT GetLockJob : public KJob
{
    Q_OBJECT

public:
    /**
      Creates a new GetLockJob.
    */
    explicit GetLockJob(QObject *parent = 0);

    /**
      Destroys the GetLockJob.
    */
    ~GetLockJob();

    /* reimpl */
    void start() Q_DECL_OVERRIDE;

private:
    class Private;
    friend class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void doStart())
    Q_PRIVATE_SLOT(d, void serviceOwnerChanged(QString, QString, QString))
    Q_PRIVATE_SLOT(d, void timeout())
};

// ===================== helper functions ============================

/**
  * Sets on @p col the required attributes of SpecialCollection type @p type
  * These are a SpecialCollectionAttribute and an EntityDisplayAttribute.
  * @param col collection
  * @param type collection type
  * @param nameForType collection name for type
  * @param iconForType collection icon for type
*/
void setCollectionAttributes(Akonadi::Collection &col, const QByteArray &type,
                             const QMap<QByteArray, QString> &nameForType,
                             const QMap<QByteArray, QString> &iconForType);

/**
  Releases the SpecialCollectionsRequestJob lock that was obtained through
  GetLockJob.
  @return Whether the lock was released successfully.
*/
bool AKONADI_TESTS_EXPORT releaseLock();

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONSHELPERJOBS_P_H
