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
#include "attributeentity.h"
#include <QHash>
#include <QSet>

using namespace Akonadi;

struct Akonadi::AttributeEntity::Private
{
    Private()
    {
    }

    ~Private()
    {
        qDeleteAll(mAttributes);
    }

    Private(const Private &other)
    {
        Q_FOREACH (Attribute *attr, other.mAttributes) {
            mAttributes.insert(attr->type(), attr->clone());
        }
        mDeletedAttributes = other.mDeletedAttributes;
    }

    QHash<QByteArray, Attribute *> mAttributes;
    QSet<QByteArray> mDeletedAttributes;
};

AttributeEntity::AttributeEntity()
    : d_ptr(new Private)
{

}

AttributeEntity::AttributeEntity(const AttributeEntity &other)
    : d_ptr(new Private)
{
    operator=(other);
}

AttributeEntity::~AttributeEntity()
{

}

Akonadi::AttributeEntity &Akonadi::AttributeEntity::operator=(const Akonadi::AttributeEntity &other)
{
    QHash<QByteArray, Attribute *>::const_iterator it = other.d_ptr->mAttributes.constBegin();
    for (; it != other.d_ptr->mAttributes.constEnd(); it++) {
        d_ptr->mAttributes.insert(it.key(), it.value()->clone());
    }
    d_ptr->mDeletedAttributes = other.d_ptr->mDeletedAttributes;
    return *this;
}

void AttributeEntity::addAttribute(Attribute *attr)
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

void AttributeEntity::removeAttribute(const QByteArray &type)
{
    d_ptr->mDeletedAttributes.insert(type);
    delete d_ptr->mAttributes.take(type);
}

bool AttributeEntity::hasAttribute(const QByteArray &type) const
{
    return d_ptr->mAttributes.contains(type);
}

Attribute::List AttributeEntity::attributes() const
{
    return d_ptr->mAttributes.values();
}

void Akonadi::AttributeEntity::clearAttributes()
{
    Q_FOREACH (Attribute *attr, d_ptr->mAttributes) {
        d_ptr->mDeletedAttributes.insert(attr->type());
        delete attr;
    }
    d_ptr->mAttributes.clear();
}

Attribute *AttributeEntity::attribute(const QByteArray &type) const
{
    if (d_ptr->mAttributes.contains(type)) {
        return d_ptr->mAttributes.value(type);
    }
    return 0;
}

QSet<QByteArray> &AttributeEntity::removedAttributes() const
{
    return d_ptr->mDeletedAttributes;
}
