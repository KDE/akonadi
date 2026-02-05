/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "specialcollections.h"
#include "transactionsequence.h"

#include <QVariant>

#include <memory>

namespace Akonadi
{
class SpecialCollectionsRequestJobPrivate;

/*!
 * \brief A job to request SpecialCollections.
 *
 * Use this job to request the SpecialCollections you need. You can request both
 * default SpecialCollections and SpecialCollections in a given resource. The default
 * SpecialCollections resource is created when the first default SpecialCollection is
 * requested, but if a SpecialCollection in a custom resource is requested, this
 * job expects that resource to exist already.
 *
 * If the folders you requested already exist, this job simply succeeds.
 * Otherwise, it creates the required collections and registers them with
 * SpecialCollections.
 *
 * This class is not meant to be used directly but as a base class for type
 * specific special collection request jobs.
 *
 * \author Constantin Berzan <exit3219@gmail.com>
 * \since 4.4
 */
class AKONADICORE_EXPORT SpecialCollectionsRequestJob : public TransactionSequence
{
    Q_OBJECT

public:
    /*!
     * Destroys the special collections request job.
     */
    ~SpecialCollectionsRequestJob() override;

    /*!
     * Requests a special collection of the given \a type in the default resource.
     */
    void requestDefaultCollection(const QByteArray &type);

    /*!
     * Requests a special collection of the given \a type in the given resource \a instance.
     */
    void requestCollection(const QByteArray &type, const AgentInstance &instance);

    /*!
     * Returns the requested collection.
     */
    [[nodiscard]] Collection collection() const;

protected:
    /*!
     * Creates a new special collections request job.
     *
     * \a collections The SpecialCollections object that shall be used.
     * \a parent The parent object.
     */
    explicit SpecialCollectionsRequestJob(SpecialCollections *collections, QObject *parent = nullptr);

    /*!
     * Sets the \a type of the resource that shall be created if the requested
     * special collection does not exist yet.
     */
    void setDefaultResourceType(const QString &type);

    /*!
     * Sets the configuration \a options that shall be applied to the new resource
     * that is created if the requested special collection does not exist yet.
     */
    void setDefaultResourceOptions(const QVariantMap &options);

    /*!
     * Sets the list of well known special collection \a types.
     */
    void setTypes(const QList<QByteArray> &types);

    /*!
     * Sets the \a map of special collection types to display names.
     */
    void setNameForTypeMap(const QMap<QByteArray, QString> &map);

    /*!
     * Sets the \a map of special collection types to icon names.
     */
    void setIconForTypeMap(const QMap<QByteArray, QString> &map);

    /* reimpl */
    void doStart() override;
    /* reimpl */
    void slotResult(KJob *job) override;

private:
    friend class SpecialCollectionsRequestJobPrivate;
    friend class DefaultResourceJobPrivate;

    std::unique_ptr<SpecialCollectionsRequestJobPrivate> const d;

    Q_PRIVATE_SLOT(d, void releaseLock())
    Q_PRIVATE_SLOT(d, void resourceScanResult(KJob *))
    Q_PRIVATE_SLOT(d, void collectionCreateResult(KJob *))
};

} // namespace Akonadi
