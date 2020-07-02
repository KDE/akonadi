/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_XML_FORMAT_P_H
#define AKONADI_XML_FORMAT_P_H

#include <QString>

namespace Akonadi
{

namespace Format
{

namespace Tag
{

inline QString root()
{
    return QStringLiteral("knut");
}
inline QString collection()
{
    return QStringLiteral("collection");
}
inline QString item()
{
    return QStringLiteral("item");
}
inline QString attribute()
{
    return QStringLiteral("attribute");
}
inline QString flag()
{
    return QStringLiteral("flag");
}
inline QString tag()
{
    return QStringLiteral("tag");
}
inline QString payload()
{
    return QStringLiteral("payload");
}

}

namespace Attr
{

inline QString remoteId()
{
    return QStringLiteral("rid");
}
inline QString attributeType()
{
    return QStringLiteral("type");
}
inline QString collectionName()
{
    return QStringLiteral("name");
}
inline QString collectionContentTypes()
{
    return QStringLiteral("content");
}
inline QString itemMimeType()
{
    return QStringLiteral("mimetype");
}
inline QString name()
{
    return QStringLiteral("name");
}
inline QString gid()
{
    return QStringLiteral("gid");
}
inline QString type()
{
    return QStringLiteral("type");
}

}

}

}

#endif
