/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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

#include "messagefolderattribute.h"

using namespace Akonadi;

class Akonadi::MessageFolderAttribute::Private
{
public:
    Private()
        : isOutboundFolder(false) { }

    bool isOutboundFolder;
};

MessageFolderAttribute::MessageFolderAttribute()
    : d(new Private)
{
}

MessageFolderAttribute::MessageFolderAttribute(const MessageFolderAttribute &other)
    : Attribute(other)
    , d(new Private(*(other.d)))
{
}

MessageFolderAttribute::~MessageFolderAttribute()
{
    delete d;
}

QByteArray MessageFolderAttribute::type() const
{
    return "MESSAGEFOLDER";
}

MessageFolderAttribute *MessageFolderAttribute::clone() const
{
    return new MessageFolderAttribute(*this);
}

QByteArray MessageFolderAttribute::serialized() const
{
    QByteArray rv;

    if (d->isOutboundFolder) {
        rv += "outbound";
    } else {
        rv += "inbound";
    }

    return rv;
}

void MessageFolderAttribute::deserialize(const QByteArray &data)
{
    if (data == "outbound") {
        d->isOutboundFolder = true;
    } else {
        d->isOutboundFolder = false;
    }
}

bool MessageFolderAttribute::isOutboundFolder() const
{
    return d->isOutboundFolder;
}

void MessageFolderAttribute::setOutboundFolder(bool outbound)
{
    d->isOutboundFolder = outbound;
}
