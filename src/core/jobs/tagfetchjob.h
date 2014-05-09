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

#ifndef AKONADI_TAGFETCHJOB_H
#define AKONADI_TAGFETCHJOB_H

#include "akonadicore_export.h"
#include "job.h"
#include "tag.h"

namespace Akonadi {

class TagFetchScope;
class TagFetchJobPrivate;

/**
 * @short Job that fetches tags.
 * @since 4.13
 */
class AKONADICORE_EXPORT TagFetchJob : public Job
{
    Q_OBJECT

public:

    TagFetchJob(QObject *parent = 0);
    TagFetchJob(const Tag &tag, QObject *parent = 0);
    TagFetchJob(const Tag::List &tags, QObject *parent = 0);
    TagFetchJob(const QList<Tag::Id> &ids, QObject *parent = 0);

    void setFetchScope(const TagFetchScope &fetchScope);

    TagFetchScope &fetchScope();

    /**
     * Returns the fetched tags after the job has been completed.
     */
    Tag::List tags() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever new tags have been fetched completely.
     *
     * @param items The fetched tags.
     */
    void tagsReceived(const Akonadi::Tag::List &tags);

protected:
    virtual void doStart();
    virtual void doHandleResponse(const QByteArray &tag, const QByteArray &data);

private:
    Q_DECLARE_PRIVATE(TagFetchJob)

    //@cond PRIVATE
    Q_PRIVATE_SLOT(d_func(), void timeout())
    //@endcond
};

}

#endif
