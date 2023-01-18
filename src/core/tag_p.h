/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
        , id(other.id)
        , gid(other.gid)
        , remoteId(other.remoteId)
        , type(other.type)
        , mAttributeStorage(other.mAttributeStorage)
    {
        if (other.parent) {
            parent.reset(new Tag(*other.parent));
        }
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
