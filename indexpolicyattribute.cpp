/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "indexpolicyattribute.h"

#include <akonadi/private/imapparser_p.h>

#include <QtCore/QString>

using namespace Akonadi;

class IndexPolicyAttribute::Private
{
public:
    Private()
        : enable(true)
    {
    }
    bool enable;
};

IndexPolicyAttribute::IndexPolicyAttribute()
    : d(new Private)
{
}

IndexPolicyAttribute::~IndexPolicyAttribute()
{
    delete d;
}

bool IndexPolicyAttribute::indexingEnabled() const
{
    return d->enable;
}

void IndexPolicyAttribute::setIndexingEnabled(bool enable)
{
    d->enable = enable;
}

QByteArray IndexPolicyAttribute::type() const
{
    static const QByteArray sType( "INDEXPOLICY" );
    return sType;
}

Attribute *IndexPolicyAttribute::clone() const
{
    IndexPolicyAttribute *attr = new IndexPolicyAttribute;
    attr->setIndexingEnabled(indexingEnabled());
    return attr;
}

QByteArray IndexPolicyAttribute::serialized() const
{
    QList<QByteArray> l;
    l.append("ENABLE");
    l.append(d->enable ? "true" : "false");
    return "(" + ImapParser::join(l, " ") + ')';   //krazy:exclude=doublequote_chars
}

void IndexPolicyAttribute::deserialize(const QByteArray &data)
{
    QList<QByteArray> l;
    ImapParser::parseParenthesizedList(data, l);
    for (int i = 0; i < l.size() - 1; i += 2) {
        const QByteArray key = l.at(i);
        if (key == "ENABLE") {
            d->enable = l.at(i + 1) == "true";
        }
    }
}
