/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>
    Copyright (c) 2015 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef TAG_P_H
#define TAG_P_H

#include "tag.h"

namespace Akonadi
{

class TagPrivate : public QSharedData
{
public:
    TagPrivate()
        : id(-1)
    {
    }

    TagPrivate(const TagPrivate &other)
        : QSharedData(other)
    {
        id = other.id;
        gid = other.gid;
        remoteId = other.remoteId;
        if (other.parent) {
            parent.reset(new Tag(*other.parent));
        }
        type = other.type;
        for (Attribute *attr : qAsConst(other.mAttributes)) {
            mAttributes.insert(attr->type(), attr->clone());
        }
        mDeletedAttributes = other.mDeletedAttributes;
    }

    ~TagPrivate()
    {
        qDeleteAll(mAttributes);
    }

    // 4 bytes padding here (after QSharedData)

    Tag::Id id;
    QByteArray gid;
    QByteArray remoteId;
    QScopedPointer<Tag> parent;
    QByteArray type;
    QHash<QByteArray, Attribute *> mAttributes;
    QSet<QByteArray> mDeletedAttributes;
};

}

#endif
