/*
    Copyright (c) 2013-2020 Laurent Montel <montel@kde.org>

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

#include "newmailnotifierattribute.h"

#include <QByteArray>
#include <QIODevice>
#include <QDataStream>

namespace Akonadi
{

class NewMailNotifierAttributePrivate
{
public:
    NewMailNotifierAttributePrivate()
        : ignoreNewMail(false)
    {
    }
    bool ignoreNewMail;
};

NewMailNotifierAttribute::NewMailNotifierAttribute()
    : d(new NewMailNotifierAttributePrivate)
{
}

NewMailNotifierAttribute::~NewMailNotifierAttribute()
{
    delete d;
}

NewMailNotifierAttribute *NewMailNotifierAttribute::clone() const
{
    NewMailNotifierAttribute *attr = new NewMailNotifierAttribute();
    attr->setIgnoreNewMail(ignoreNewMail());
    return attr;
}

QByteArray NewMailNotifierAttribute::type() const
{
    static const QByteArray sType("newmailnotifierattribute");
    return sType;
}

QByteArray NewMailNotifierAttribute::serialized() const
{
    QByteArray result;
    QDataStream s(&result, QIODevice::WriteOnly);
    s << ignoreNewMail();
    return result;
}

void NewMailNotifierAttribute::deserialize(const QByteArray &data)
{
    QDataStream s(data);
    s >> d->ignoreNewMail;
}

bool NewMailNotifierAttribute::ignoreNewMail() const
{
    return d->ignoreNewMail;
}

void NewMailNotifierAttribute::setIgnoreNewMail(bool b)
{
    d->ignoreNewMail = b;
}

bool NewMailNotifierAttribute::operator==(const NewMailNotifierAttribute &other) const
{
    return d->ignoreNewMail == other.ignoreNewMail();
}
}
