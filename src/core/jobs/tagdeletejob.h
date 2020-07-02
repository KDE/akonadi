/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TAGDELETEJOB_H
#define AKONADI_TAGDELETEJOB_H

#include "akonadicore_export.h"
#include "job.h"
#include "tag.h"

namespace Akonadi
{

class Tag;
class TagDeleteJobPrivate;

/**
 * @short Job that deletes tags.
 * @since 4.13
 */
class AKONADICORE_EXPORT TagDeleteJob : public Job
{
    Q_OBJECT

public:
    explicit TagDeleteJob(const Tag &tag, QObject *parent = nullptr);
    explicit TagDeleteJob(const Tag::List &tag, QObject *parent = nullptr);

    /**
     * Returns the tags passed to the constructor.
     */
    Q_REQUIRED_RESULT Tag::List tags() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(TagDeleteJob)
};

}

#endif
