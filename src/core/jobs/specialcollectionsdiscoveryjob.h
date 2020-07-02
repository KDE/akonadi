/*
    SPDX-FileCopyrightText: 2013 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_SPECIALCOLLECTIONSDISCOVERYJOB_H
#define AKONADI_SPECIALCOLLECTIONSDISCOVERYJOB_H

#include "akonadicore_export.h"
#include "collection.h"
#include "specialcollections.h"
#include <KCompositeJob>

namespace Akonadi
{

class SpecialCollectionsDiscoveryJobPrivate;

/**
 * @short A job to discover all SpecialCollections.
 *
 * The collections get registered into SpecialCollections.
 *
 * This class is not meant to be used directly but as a base class for type
 * specific special collection request jobs.
 *
 * @author David Faure <faure@kde.org>
 * @since 4.11
*/
class AKONADICORE_EXPORT SpecialCollectionsDiscoveryJob : public KCompositeJob
{
    Q_OBJECT

public:

    /**
     * Destroys the special collections request job.
     */
    ~SpecialCollectionsDiscoveryJob() override;

    void start() override;

protected:
    /**
     * Creates a new special collections request job.
     *
     * @param collections The SpecialCollections object that shall be used.
     * @param parent The parent object.
     */
    explicit SpecialCollectionsDiscoveryJob(SpecialCollections *collections, const QStringList &mimeTypes, QObject *parent = nullptr);

    /* reimpl */
    void slotResult(KJob *job) override;

private:
    //@cond PRIVATE
    SpecialCollectionsDiscoveryJobPrivate *const d;
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONSDISCOVERYJOB_H
