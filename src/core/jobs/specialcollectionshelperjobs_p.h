/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonaditests_export.h"
#include "collection.h"
#include "specialcollections.h"
#include "transactionsequence.h"

#include <QVariant>

#include <memory>

namespace Akonadi
{
// ===================== ResourceScanJob ============================

class ResourceScanJobPrivate;

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
    explicit ResourceScanJob(const QString &resourceId, KCoreConfigSkeleton *settings, QObject *parent = nullptr);

    /**
      Destroys this ResourceScanJob.
    */
    ~ResourceScanJob() override;

    /**
      Returns the resource ID of the resource being scanned.
    */
    Q_REQUIRED_RESULT QString resourceId() const;

    /**
      Sets the resource ID of the resource to scan.
    */
    void setResourceId(const QString &resourceId);

    /**
      Returns the root collection of the resource being scanned.
      This function relies on there being a single top-level collection owned
      by this resource.
    */
    Q_REQUIRED_RESULT Akonadi::Collection rootResourceCollection() const;

    /**
      Returns all the collections of this resource which have a
      SpecialCollectionAttribute. These might include the root resource collection.
    */
    Q_REQUIRED_RESULT Akonadi::Collection::List specialCollections() const;

protected:
    /* reimpl */
    void doStart() override;

private:
    friend class ResourceScanJobPrivate;
    std::unique_ptr<ResourceScanJobPrivate> const d;
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
    explicit DefaultResourceJob(KCoreConfigSkeleton *settings, QObject *parent = nullptr);

    /**
     * Destroys the DefaultResourceJob.
     */
    ~DefaultResourceJob() override;

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
    void doStart() override;
    /* reimpl */
    void slotResult(KJob *job) override;

private:
    friend class DefaultResourceJobPrivate;
    std::unique_ptr<DefaultResourceJobPrivate> const d;
};

// ===================== GetLockJob ============================
class GetLockJobPrivate;

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
    explicit GetLockJob(QObject *parent = nullptr);

    /**
      Destroys the GetLockJob.
    */
    ~GetLockJob() override;

    /* reimpl */
    void start() override;

private:
    friend class GetLockJobPrivate;
    std::unique_ptr<GetLockJobPrivate> const d;

    Q_PRIVATE_SLOT(d, void doStart())
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
void setCollectionAttributes(Akonadi::Collection &col,
                             const QByteArray &type,
                             const QMap<QByteArray, QString> &nameForType,
                             const QMap<QByteArray, QString> &iconForType);

/**
  Releases the SpecialCollectionsRequestJob lock that was obtained through
  GetLockJob.
  @return Whether the lock was released successfully.
*/
bool AKONADI_TESTS_EXPORT releaseLock();

} // namespace Akonadi
