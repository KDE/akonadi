/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef FAKEENTITIES_H
#define FAKEENTITIES_H

#include "entities.h"

namespace Akonadi {
namespace Server {

class FakePart : public Part
{
public:
    FakePart()
        : Part()
    {
    }

    void setPartType(const PartType &partType)
    {
        m_partType = partType;
        Part::setPartType(partType);
    }

    PartType partType() const
    {
        return m_partType;
    }

private:
    PartType m_partType;
};

class FakeTag : public Tag
{
public:
    FakeTag()
        : Tag()
    {
    }

    void setTagType(const TagType &tagType)
    {
        m_tagType = tagType;
        Tag::setTagType(tagType);
    }

    TagType tagType() const
    {
        return m_tagType;
    }

    void setRemoteId(const QString &remoteId)
    {
        m_remoteId = remoteId;
    }

    QString remoteId() const
    {
        return m_remoteId;
    }

private:
    TagType m_tagType;
    QString m_remoteId;
};

} // namespace Server
} // namespace Akonadi

#endif // FAKEENTITIES_H
