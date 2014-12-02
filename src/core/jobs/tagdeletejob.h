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

#ifndef AKONADI_TAGDELETEJOB_H
#define AKONADI_TAGDELETEJOB_H

#include "akonadicore_export.h"
#include "job.h"
#include "tag.h"

namespace Akonadi {

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
    explicit TagDeleteJob(const Tag &tag, QObject *parent = Q_NULLPTR);
    explicit TagDeleteJob(const Tag::List &tag, QObject *parent = Q_NULLPTR);

    /**
     * Returns the tags passed to the constructor.
     */
    Tag::List tags() const;

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(TagDeleteJob)
};

}

#endif
