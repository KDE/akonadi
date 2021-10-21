/*
    SPDX-FileCopyrightText: 2007-2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "attributefactory.h"

#include "collectionidentificationattribute.h"
#include "collectionquotaattribute.h"
#include "collectionrightsattribute_p.h"
#include "entityannotationsattribute.h"
#include "entitydeletedattribute.h"
#include "entitydisplayattribute.h"
#include "entityhiddenattribute.h"
#include "favoritecollectionattribute.h"
#include "indexpolicyattribute.h"
#include "persistentsearchattribute.h"
#include "tagattribute.h"

#include <QHash>

#include <unordered_map>

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

    void deserialize(const QByteArray &data) override
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
    void init()
    {
        if (!initialized) {
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
            AttributeFactory::registerAttribute<CollectionIdentificationAttribute>();
        }
    }
    bool initialized = false;
};

Q_GLOBAL_STATIC(StaticAttributeFactory, s_attributeInstance) // NOLINT(readability-redundant-member-init)

} // namespace Internal

using Akonadi::Internal::s_attributeInstance;

/**
 * @internal
 */
class AttributeFactoryPrivate
{
public:
    std::unordered_map<QByteArray, std::unique_ptr<Attribute>> attributes;
};

AttributeFactory *AttributeFactory::self()
{
    s_attributeInstance->init();
    return s_attributeInstance;
}

AttributeFactory::AttributeFactory()
    : d(new AttributeFactoryPrivate())
{
}

AttributeFactory::~AttributeFactory() = default;

void AttributeFactory::registerAttribute(std::unique_ptr<Attribute> attr)
{
    Q_ASSERT(attr);
    Q_ASSERT(!attr->type().contains(' ') && !attr->type().contains('\'') && !attr->type().contains('"'));
    auto it = d->attributes.find(attr->type());
    if (it != d->attributes.end()) {
        d->attributes.erase(it);
    }
    d->attributes.emplace(attr->type(), std::move(attr));
}

Attribute *AttributeFactory::createAttribute(const QByteArray &type)
{
    auto attr = self()->d->attributes.find(type);
    if (attr == self()->d->attributes.cend()) {
        return new Internal::DefaultAttribute(type);
    }

    return attr->second->clone();
}

} // namespace Akonadi
