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

#ifndef ENTITY_P_H
#define ENTITY_P_H

#include "entity.h"
#include "collection.h"

#include <QtCore/QSet>
#include <QtCore/QSharedData>
#include <QtCore/QString>

#define AKONADI_DEFINE_PRIVATE( Class ) \
Class##Private *Class ::d_func() \
{\
    return reinterpret_cast<Class##Private *>( d_ptr.data() );\
} \
const Class##Private *Class ::d_func() const \
{\
    return reinterpret_cast<const Class##Private *>( d_ptr.data() );\
}

namespace Akonadi {

/**
 * @internal
 */
class EntityPrivate : public QSharedData
{
public:
    explicit EntityPrivate(Entity::Id id = -1)
        : mId(id)
        , mParent(0)
    {
    }

    virtual ~EntityPrivate()
    {
        qDeleteAll(mAttributes);
        delete mParent;
    }

    EntityPrivate(const EntityPrivate &other)
        : QSharedData(other)
        , mParent(0)
    {
        mId = other.mId;
        mRemoteId = other.mRemoteId;
        mRemoteRevision = other.mRemoteRevision;
        foreach (Attribute *attr, other.mAttributes) {
            mAttributes.insert(attr->type(), attr->clone());
        }
        mDeletedAttributes = other.mDeletedAttributes;
        if (other.mParent) {
            mParent = new Collection(*(other.mParent));
        }
    }

    virtual void resetChangeLog()
    {
        mDeletedAttributes.clear();
    }

    virtual EntityPrivate *clone() const = 0;

    Entity::Id mId;
    QString mRemoteId;
    QString mRemoteRevision;
    QHash<QByteArray, Attribute *> mAttributes;
    QSet<QByteArray> mDeletedAttributes;
    mutable Collection *mParent;
};

}

/**
 * @internal
 *
 * This template specialization is used to change the detach
 * behaviour of QSharedDataPointer to allow 'virtual copy constructors',
 * so Akonadi::ItemPrivate and Akonadi::CollectionPrivate are copied correctly.
 */
template <>
Q_INLINE_TEMPLATE Akonadi::EntityPrivate *QSharedDataPointer<Akonadi::EntityPrivate>::clone()
{
    return d->clone();
}

#endif
