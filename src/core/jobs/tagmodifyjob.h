/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
class Tag;
class TagModifyJobPrivate;

/*!
 * \brief Job that modifies a tag in the Akonadi storage.
 * \since 4.13
 */
class AKONADICORE_EXPORT TagModifyJob : public Job
{
    Q_OBJECT

public:
    /*!
     * Creates a new tag modify job.
     *
     * \a tag The tag to modify.
     * \a parent The parent object.
     */
    explicit TagModifyJob(const Tag &tag, QObject *parent = nullptr);

    /*!
     * Returns the modified tag.
     */
    [[nodiscard]] Tag tag() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(TagModifyJob)
};

}
