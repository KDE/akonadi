/*
    Copyright (c) 2007 - 2008 Volker Krause <vkrause@kde.org>

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

#include "attributefactory.h"

#include "collectionquotaattribute.h"
#include "collectionrightsattribute_p.h"
#include "entitydisplayattribute.h"
#include "entityhiddenattribute.h"
#include "indexpolicyattribute.h"
#include "persistentsearchattribute.h"
#include "entitydeletedattribute.h"
#include "tagattribute.h"
#include "entityannotationsattribute.h"

#include <QtCore/QHash>

using namespace Akonadi;

namespace Akonadi {
namespace Internal {

/**
 * @internal
 */
class DefaultAttribute : public Attribute
{
public:
    explicit DefaultAttribute(const QByteArray &type, const QByteArray &value = QByteArray())
        : mType(type)
        , mValue(value)
    {
    }

    QByteArray type() const
    {
        return mType;
    }
    Attribute *clone() const
    {
        return new DefaultAttribute(mType, mValue);
    }

    QByteArray serialized() const
    {
        return mValue;
    }
    void deserialize(const QByteArray &data)
    {
        mValue = data;
    }

private:
    QByteArray mType, mValue;
};

/**
 * @internal
 */
class StaticAttributeFactory : public AttributeFactory
{
public:
    StaticAttributeFactory()
        : AttributeFactory()
        , initialized(false)
    {
    }
    void init()
    {
        if (initialized) {
            return;
        }
        initialized = true;

        // Register built-in attributes
        AttributeFactory::registerAttribute<CollectionQuotaAttribute>();
        AttributeFactory::registerAttribute<CollectionRightsAttribute>();
        AttributeFactory::registerAttribute<EntityDisplayAttribute>();
        AttributeFactory::registerAttribute<EntityHiddenAttribute>();
        AttributeFactory::registerAttribute<IndexPolicyAttribute>();
        AttributeFactory::registerAttribute<PersistentSearchAttribute>();
        AttributeFactory::registerAttribute<EntityDeletedAttribute>();
        AttributeFactory::registerAttribute<EntityAnnotationsAttribute>();
        AttributeFactory::registerAttribute<TagAttribute>();
    }
    bool initialized;
};

Q_GLOBAL_STATIC(StaticAttributeFactory, s_attributeInstance)

}

using Akonadi::Internal::s_attributeInstance;

/**
 * @internal
 */
class AttributeFactory::Private
{
public:
    QHash<QByteArray, Attribute *> attributes;
};

AttributeFactory *AttributeFactory::self()
{
    s_attributeInstance->init();
    return s_attributeInstance;
}

AttributeFactory::AttributeFactory()
    : d(new Private)
{
}

AttributeFactory::~ AttributeFactory()
{
    qDeleteAll(d->attributes);
    delete d;
}

void AttributeFactory::registerAttribute(Attribute *attr)
{
    Q_ASSERT(attr);
    Q_ASSERT(!attr->type().contains(' ') && !attr->type().contains('\'') && !attr->type().contains('"'));
    QHash<QByteArray, Attribute *>::Iterator it = d->attributes.find(attr->type());
    if (it != d->attributes.end()) {
        delete *it;
        d->attributes.erase(it);
    }
    d->attributes.insert(attr->type(), attr);
}

Attribute *AttributeFactory::createAttribute(const QByteArray &type)
{
    Attribute *attr = self()->d->attributes.value(type);
    if (attr) {
        return attr->clone();
    }
    return new Internal::DefaultAttribute(type);
}

}
