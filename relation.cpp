/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "relation.h"

#include <akonadi/item.h>

using namespace Akonadi;

const char *Akonadi::Relation::GENERIC = "GENERIC";

struct Relation::Private {
    Item left;
    Item right;
    QByteArray type;
    QByteArray remoteId;
};

Relation::Relation()
    : d(new Private)
{

}

Relation::Relation(const QByteArray &type, const Item &left, const Item &right)
    : d(new Private)
{
    d->left = left;
    d->right = right;
    d->type = type;
}

Relation::Relation(const Relation &other)
    : d(new Private)
{
    operator=(other);
}

Relation::~Relation()
{
}

Relation &Relation::operator=(const Relation &other)
{
    d->left = other.d->left;
    d->right = other.d->right;
    d->type = other.d->type;
    return *this;
}

bool Relation::operator==(const Relation &other) const
{
    if (isValid() && other.isValid()) {
        return d->left == other.d->left
            && d->right == other.d->right
            && d->type == other.d->type;
    }
    return false;
}

bool Relation::operator!=(const Relation &other) const
{
    return !operator==(other);
}

void Relation::setLeft(const Item &left)
{
    d->left = left;
}

Item Relation::left() const
{
    return d->left;
}

void Relation::setRight(const Item &right)
{
    d->right = right;
}

Item Relation::right() const
{
    return d->right;
}

void Relation::setType(const QByteArray &type) const
{
    d->type = type;
}

QByteArray Relation::type() const
{
    return d->type;
}

void Relation::setRemoteId(const QByteArray &remoteId) const
{
    d->remoteId = remoteId;
}

QByteArray Relation::remoteId() const
{
    return d->remoteId;
}

bool Relation::isValid() const
{
    return (d->left.isValid() || !d->left.remoteId().isEmpty()) && (d->right.isValid() || !d->left.remoteId().isEmpty()) && !d->type.isEmpty();
}

uint qHash(const Relation &relation)
{
    return (3*qHash(relation.left())+qHash(relation.right())+qHash(relation.type()));
}

QDebug &operator<<(QDebug &debug, const Relation &relation)
{
    debug << "Akonadi::Relation( TYPE " << relation.type() << ", LEFT " << relation.left().id() << ", RIGHT " << relation.right().id() << ")";
    return debug;
}
