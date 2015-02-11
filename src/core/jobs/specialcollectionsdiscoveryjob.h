/*
    Copyright (c) 2013 David Faure <faure@kde.org>

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

#ifndef AKONADI_SPECIALCOLLECTIONSDISCOVERYJOB_H
#define AKONADI_SPECIALCOLLECTIONSDISCOVERYJOB_H

#include "akonadicore_export.h"
#include "collection.h"
#include "specialcollections.h"
#include <kcompositejob.h>

namespace Akonadi {

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
    ~SpecialCollectionsDiscoveryJob();

    virtual void start() Q_DECL_OVERRIDE;

protected:
    /**
     * Creates a new special collections request job.
     *
     * @param collections The SpecialCollections object that shall be used.
     * @param parent The parent object.
     */
    explicit SpecialCollectionsDiscoveryJob(SpecialCollections *collections, const QStringList &mimeTypes, QObject *parent = Q_NULLPTR);

    /* reimpl */
    void slotResult(KJob *job) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    SpecialCollectionsDiscoveryJobPrivate *const d;
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONSDISCOVERYJOB_H
