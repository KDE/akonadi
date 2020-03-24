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

#include "attributestorage_p.h"
#include "tag.h"

namespace Akonadi
{

class TagPrivate : public QSharedData
{
public:
    explicit TagPrivate() = default;
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
        mAttributeStorage = other.mAttributeStorage;
    }

    ~TagPrivate() = default;

    void resetChangeLog()
    {
        mAttributeStorage.resetChangeLog();
    }

    // 4 bytes padding here (after QSharedData)

    Tag::Id id = -1;
    QByteArray gid;
    QByteArray remoteId;
    QScopedPointer<Tag> parent;
    QByteArray type;
    AttributeStorage mAttributeStorage;
};

}

#endif
