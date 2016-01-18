/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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
