/*
    Copyright (c) 2007 - 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
#include "favoritecollectionattribute.h"

#include <QHash>

using namespace Akonadi;

namespace Akonadi
{
namespace Internal
{

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

    DefaultAttribute(const DefaultAttribute &) = delete;
    DefaultAttribute &operator=(const DefaultAttribute &) = delete;

    QByteArray type() const override
    {
        return mType;
    }
    Attribute *clone() const override
    {
        return new DefaultAttribute(mType, mValue);
    }

    QByteArray serialized() const override
    {
        return mValue;
    }
    void deserialize(const QByteArray &data) override {
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
        : initialized(false)
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
        AttributeFactory::registerAttribute<FavoriteCollectionAttribute>();
    }
    bool initialized;
};

Q_GLOBAL_STATIC(StaticAttributeFactory, s_attributeInstance) // NOLINT(readability-redundant-member-init)

} // namespace Internal

using Akonadi::Internal::s_attributeInstance;

/**
 * @internal
 */
class Q_DECL_HIDDEN AttributeFactory::Private
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

AttributeFactory::~AttributeFactory()
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

} // namespace Akonadi
