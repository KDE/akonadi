/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#include "entity.h"
#include "entity_p.h"
#include "collection.h"

using namespace Akonadi;

Q_GLOBAL_STATIC(Akonadi::Collection, s_defaultParentCollection)

/**
 * Helper method for assignment operator and copy constructor.
 */
static void assignEntityPrivate(QSharedDataPointer<EntityPrivate> &one, const QSharedDataPointer<EntityPrivate> &other)
{
    // We can't simply do one = other here, we have to use a temp.
    // Otherwise ProtocolHelperTest::testParentCollectionAfterCollectionParsing()
    // will break.
    //
    // The reason are assignments like
    //   col = col.parentCollection()
    //
    // Here, parentCollection() actually returns a reference to a pointer owned
    // by col. So when col (or rather, it's private class) is deleted, the pointer
    // to the parent collection and therefore the reference becomes invalid.
    //
    // With a single-line assignment here, the parent collection would be deleted
    // before it is assigned, and therefore the resulting object would point to
    // uninitalized memory.
    QSharedDataPointer<EntityPrivate> temp = other;
    one = temp;
}

Entity::Entity(const Entity &other)
{
    assignEntityPrivate(d_ptr, other.d_ptr);
}

Entity::Entity(EntityPrivate *dd)
    : d_ptr(dd)
{
}

Entity::~Entity()
{
}

void Entity::setId(Id id)
{
    d_ptr->mId = id;
}

Entity::Id Entity::id() const
{
    return d_ptr->mId;
}

void Entity::setRemoteId(const QString &id)
{
    d_ptr->mRemoteId = id;
}

QString Entity::remoteId() const
{
    return d_ptr->mRemoteId;
}

void Entity::setRemoteRevision(const QString &revision)
{
    d_ptr->mRemoteRevision = revision;
}

QString Entity::remoteRevision() const
{
    return d_ptr->mRemoteRevision;
}

bool Entity::isValid() const
{
    return (d_ptr->mId >= 0);
}

bool Entity::operator==(const Entity &other) const
{
    // Invalid items are the same, no matter what their internal ID is
    return (!isValid() && !other.isValid()) || (d_ptr->mId == other.d_ptr->mId);
}

bool Akonadi::Entity::operator!=(const Entity &other) const
{
    return (isValid() || other.isValid()) && (d_ptr->mId != other.d_ptr->mId);
}

Entity &Entity ::operator=(const Entity &other)
{
    if (this != &other) {
        assignEntityPrivate(d_ptr, other.d_ptr);
    }

    return *this;
}

bool Akonadi::Entity::operator<(const Entity &other) const
{
    return d_ptr->mId < other.d_ptr->mId;
}

void Entity::addAttribute(Attribute *attr)
{
    if (d_ptr->mAttributes.contains(attr->type())) {
        Attribute *existing = d_ptr->mAttributes.value(attr->type());
        if (attr == existing) {
            return;
        }
        d_ptr->mAttributes.remove(attr->type());
        delete existing;
    }
    d_ptr->mAttributes.insert(attr->type(), attr);
    d_ptr->mDeletedAttributes.remove(attr->type());
}

void Entity::removeAttribute(const QByteArray &type)
{
    d_ptr->mDeletedAttributes.insert(type);
    delete d_ptr->mAttributes.take(type);
}

bool Entity::hasAttribute(const QByteArray &type) const
{
    return d_ptr->mAttributes.contains(type);
}

Attribute::List Entity::attributes() const
{
    return d_ptr->mAttributes.values();
}

void Akonadi::Entity::clearAttributes()
{
    foreach (Attribute *attr, d_ptr->mAttributes) {
        d_ptr->mDeletedAttributes.insert(attr->type());
        delete attr;
    }
    d_ptr->mAttributes.clear();
}

Attribute *Entity::attribute(const QByteArray &type) const
{
    if (d_ptr->mAttributes.contains(type)) {
        return d_ptr->mAttributes.value(type);
    }
    return 0;
}

uint qHash(const Akonadi::Entity &entity)
{
    return qHash(entity.id());
}

Collection &Entity::parentCollection()
{
    if (!d_ptr->mParent) {
        d_ptr->mParent = new Collection();
    }
    return *(d_ptr->mParent);
}

Collection Entity::parentCollection() const
{
    if (!d_ptr->mParent) {
        return *(s_defaultParentCollection);
    } else {
        return *(d_ptr->mParent);
    }
}

void Entity::setParentCollection(const Collection &parent)
{
    delete d_ptr->mParent;
    d_ptr->mParent = new Collection(parent);
}

AKONADI_DEFINE_PRIVATE(Entity)
