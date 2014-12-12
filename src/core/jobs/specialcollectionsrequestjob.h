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

#ifndef AKONADI_SPECIALCOLLECTIONSREQUESTJOB_H
#define AKONADI_SPECIALCOLLECTIONSREQUESTJOB_H

#include "akonadicore_export.h"
#include "collection.h"
#include "specialcollections.h"
#include "transactionsequence.h"

#include <QtCore/QVariant>

namespace Akonadi {

class SpecialCollectionsRequestJobPrivate;

/**
 * @short A job to request SpecialCollections.
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
 * @author Constantin Berzan <exit3219@gmail.com>
 * @since 4.4
*/
class AKONADICORE_EXPORT SpecialCollectionsRequestJob : public TransactionSequence
{
    Q_OBJECT

public:

    /**
     * Destroys the special collections request job.
     */
    ~SpecialCollectionsRequestJob();

    /**
     * Requests a special collection of the given @p type in the default resource.
     */
    void requestDefaultCollection(const QByteArray &type);

    /**
     * Requests a special collection of the given @p type in the given resource @p instance.
     */
    void requestCollection(const QByteArray &type, const AgentInstance &instance);

    /**
     * Returns the requested collection.
     */
    Collection collection() const;

protected:
    /**
     * Creates a new special collections request job.
     *
     * @param collections The SpecialCollections object that shall be used.
     * @param parent The parent object.
     */
    explicit SpecialCollectionsRequestJob(SpecialCollections *collections, QObject *parent = Q_NULLPTR);

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

    /* reimpl */
    void doStart() Q_DECL_OVERRIDE;
    /* reimpl */
    void slotResult(KJob *job) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    friend class SpecialCollectionsRequestJobPrivate;
    friend class DefaultResourceJobPrivate;

    SpecialCollectionsRequestJobPrivate *const d;

    Q_PRIVATE_SLOT(d, void lockResult(KJob *))
    Q_PRIVATE_SLOT(d, void releaseLock())
    Q_PRIVATE_SLOT(d, void resourceScanResult(KJob *))
    Q_PRIVATE_SLOT(d, void collectionCreateResult(KJob *))
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONSREQUESTJOB_H
