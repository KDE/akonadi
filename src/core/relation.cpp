/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "relation.h"

#include "item.h"

using namespace Akonadi;

const char *Akonadi::Relation::GENERIC = "GENERIC";

struct Q_DECL_HIDDEN Relation::Private : public QSharedData {
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

Relation::Relation(const Relation &other) = default;

Relation::Relation(Relation &&) noexcept = default;

Relation::~Relation() = default;

Relation &Relation::operator=(const Relation &) = default;

Relation &Relation::operator=(Relation &&) noexcept = default;

bool Relation::operator==(const Relation &other) const
{
    if (isValid() && other.isValid()) {
        return d->left == other.d->left
               && d->right == other.d->right
               && d->type == other.d->type
               && d->remoteId == other.d->remoteId;
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

void Relation::setType(const QByteArray &type)
{
    d->type = type;
}

QByteArray Relation::type() const
{
    return d->type;
}

void Relation::setRemoteId(const QByteArray &remoteId)
{
    d->remoteId = remoteId;
}

QByteArray Relation::remoteId() const
{
    return d->remoteId;
}

bool Relation::isValid() const
{
    return (d->left.isValid() || !d->left.remoteId().isEmpty()) && (d->right.isValid() || !d->right.remoteId().isEmpty()) && !d->type.isEmpty();
}

uint Akonadi::qHash(const Relation &relation)
{
    return (3 * qHash(relation.left()) + qHash(relation.right()) + qHash(relation.type()) + qHash(relation.remoteId()));
}

QDebug &operator<<(QDebug &debug, const Relation &relation)
{
    debug << "Akonadi::Relation( TYPE " << relation.type() << ", LEFT " << relation.left().id() << ", RIGHT " << relation.right().id() << ", REMOTEID " << relation.remoteId() << ")";
    return debug;
}
