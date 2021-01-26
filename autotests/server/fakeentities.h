/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FAKEENTITIES_H
#define FAKEENTITIES_H

#include "entities.h"

namespace Akonadi
{
namespace Server
{
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
