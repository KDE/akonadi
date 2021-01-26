/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TAGCREATEJOB_H
#define AKONADI_TAGCREATEJOB_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
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
    explicit TagCreateJob(const Tag &tag, QObject *parent = nullptr);

    /**
     * Returns the created tag with the new unique id, or an invalid tag if the job failed.
     */
    Q_REQUIRED_RESULT Tag tag() const;

    /**
     * Merges the tag by GID if it is already existing, and returns the merged version.
     * This is false by default.
     *
     * Note that the returned tag does not contain attributes.
     */
    void setMergeIfExisting(bool merge);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(TagCreateJob)
};

}

#endif
