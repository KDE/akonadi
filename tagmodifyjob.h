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

#ifndef AKONADI_TAGMODIFYJOB_H
#define AKONADI_TAGMODIFYJOB_H

#include <akonadi/job.h>

namespace Akonadi {

class Tag;
class TagModifyJobPrivate;

/**
 * @short Job that modifies a tag in the Akonadi storage.
 * @since 4.13
 */
class AKONADI_EXPORT TagModifyJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new tag modify job.
     *
     * @param tag The tag to modify.
     * @param parent The parent object.
     */
    explicit TagModifyJob(const Tag &tag, QObject *parent = 0);

    /**
     * Returns the modified tag.
     */
    Tag tag() const;

protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

private:
    Q_DECLARE_PRIVATE(TagModifyJob)
};

}

#endif
