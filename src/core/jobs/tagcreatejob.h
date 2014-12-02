/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef AKONADI_TAGCREATEJOB_H
#define AKONADI_TAGCREATEJOB_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi {

class Tag;
class TagCreateJobPrivate;

/**
 * @short Job that creates a new tag in the Akonadi storage.
 * @since 4.13
 */
class AKONADICORE_EXPORT TagCreateJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new tag create job.
     *
     * @param tag The tag to create.
     * @param parent The parent object.
     */
    explicit TagCreateJob(const Tag &tag, QObject *parent = Q_NULLPTR);

    /**
     * Returns the created tag with the new unique id, or an invalid tag if the job failed.
     */
    Tag tag() const;

    /**
     * Merges the tag by GID if it is already existing, and returns the merged version.
     * This is false by default.
     *
     * Note that the returned tag does not contain attributes.
     */
    void setMergeIfExisting(bool merge);

protected:
    void doStart() Q_DECL_OVERRIDE;
    void doHandleResponse(const QByteArray &tag, const QByteArray &data) Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(TagCreateJob)
};

}

#endif
