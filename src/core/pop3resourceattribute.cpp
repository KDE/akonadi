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

#include "pop3resourceattribute.h"

#include <QByteArray>
#include <QIODevice>
#include <QDataStream>
namespace Akonadi
{

class Pop3ResourceAttributePrivate
{
public:
    Pop3ResourceAttributePrivate()
    {
    }
    QString accountName;
};

Pop3ResourceAttribute::Pop3ResourceAttribute()
    : d(new Pop3ResourceAttributePrivate)
{
}

Pop3ResourceAttribute::~Pop3ResourceAttribute()
{
    delete d;
}

Pop3ResourceAttribute *Pop3ResourceAttribute::clone() const
{
    Pop3ResourceAttribute *attr = new Pop3ResourceAttribute();
    attr->setPop3AccountName(pop3AccountName());
    return attr;
}

QByteArray Pop3ResourceAttribute::type() const
{
    static const QByteArray sType("pop3resourceattribute");
    return sType;
}

QByteArray Pop3ResourceAttribute::serialized() const
{
    QByteArray result;
    QDataStream s(&result, QIODevice::WriteOnly);
    s << pop3AccountName();
    return result;
}

void Pop3ResourceAttribute::deserialize(const QByteArray &data)
{
    QDataStream s(data);
    QString value;
    s >> value;
    d->accountName = value;
}

QString Pop3ResourceAttribute::pop3AccountName() const
{
    return d->accountName;
}

void Pop3ResourceAttribute::setPop3AccountName(const QString &accountName)
{
    d->accountName = accountName;
}

bool Pop3ResourceAttribute::operator==(const Pop3ResourceAttribute &other) const
{
    return d->accountName == other.pop3AccountName();
}
}
