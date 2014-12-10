/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "messagethreadingattribute.h"

using namespace Akonadi;

class Akonadi::MessageThreadingAttribute::Private
{
public:
    QList<Item::Id> perfectParents;
    QList<Item::Id> unperfectParents;
    QList<Item::Id> subjectParents;
};

MessageThreadingAttribute::MessageThreadingAttribute()
    : d(new Private)
{
}

MessageThreadingAttribute::MessageThreadingAttribute(const MessageThreadingAttribute &other)
    : Attribute(other)
    , d(new Private(*(other.d)))
{
}

MessageThreadingAttribute::~ MessageThreadingAttribute()
{
    delete d;
}

QByteArray MessageThreadingAttribute::type() const
{
    static const QByteArray sType( "MESSAGETHREADING" );
    return sType;
}

MessageThreadingAttribute *MessageThreadingAttribute::clone() const
{
    return new MessageThreadingAttribute(*this);
}

QByteArray MessageThreadingAttribute::serialized() const
{
    QByteArray rv;
    foreach (const Item::Id id, d->perfectParents) {
        rv += QByteArray::number(id) + ',';
    }
    if (!d->perfectParents.isEmpty()) {
        rv[rv.size() - 1] = ';';
    } else {
        rv += ';';
    }
    foreach (const Item::Id id, d->unperfectParents) {
        rv += QByteArray::number(id) + ',';
    }
    if (!d->unperfectParents.isEmpty()) {
        rv[rv.size() - 1] = ';';
    } else {
        rv += ';';
    }
    foreach (const Item::Id id, d->subjectParents) {
        rv += QByteArray::number(id) + ',';
    }
    if (!d->perfectParents.isEmpty()) {
        rv.chop(1);
    }

    return rv;
}

static void parseIdList(const QByteArray &data, QList<Item::Id> &result)
{
    bool ok = false;
    foreach (const QByteArray &s, data.split(',')) {
        Item::Id id = s.toLongLong(&ok);
        if (!ok) {
            continue;
        }
        result << id;
    }
}

void MessageThreadingAttribute::deserialize(const QByteArray &data)
{
    d->perfectParents.clear();
    d->unperfectParents.clear();
    d->subjectParents.clear();

    QList<QByteArray> lists = data.split(';');
    if (lists.size() != 3) {
        return;
    }

    parseIdList(lists[0], d->perfectParents);
    parseIdList(lists[1], d->unperfectParents);
    parseIdList(lists[2], d->subjectParents);
}

QList< Item::Id > MessageThreadingAttribute::perfectParents() const
{
    return d->perfectParents;
}

void MessageThreadingAttribute::setPerfectParents(const QList< Item::Id > &parents)
{
    d->perfectParents = parents;
}

QList< Item::Id > MessageThreadingAttribute::unperfectParents() const
{
    return d->unperfectParents;
}

void MessageThreadingAttribute::setUnperfectParents(const QList< Item::Id > &parents)
{
    d->unperfectParents = parents;
}

QList< Item::Id > MessageThreadingAttribute::subjectParents() const
{
    return d->subjectParents;
}

void MessageThreadingAttribute::setSubjectParents(const QList< Item::Id > &parents)
{
    d->subjectParents = parents;
}
