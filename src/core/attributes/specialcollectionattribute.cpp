/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "specialcollectionattribute.h"
#include "attributefactory.h"

using namespace Akonadi;

/**
  @internal
*/
class Q_DECL_HIDDEN SpecialCollectionAttribute::Private
{
public:
    QByteArray mType;
};

SpecialCollectionAttribute::SpecialCollectionAttribute(const QByteArray &type)
    : d(std::make_unique<Private>())
{
    d->mType = type;
}

SpecialCollectionAttribute::~SpecialCollectionAttribute() = default;

SpecialCollectionAttribute *SpecialCollectionAttribute::clone() const
{
    return new SpecialCollectionAttribute(d->mType);
}

QByteArray SpecialCollectionAttribute::type() const
{
    static const QByteArray sType("SpecialCollectionAttribute");
    return sType;
}

QByteArray SpecialCollectionAttribute::serialized() const
{
    return d->mType;
}

void SpecialCollectionAttribute::deserialize(const QByteArray &data)
{
    d->mType = data;
}

void SpecialCollectionAttribute::setCollectionType(const QByteArray &type)
{
    d->mType = type;
}

QByteArray SpecialCollectionAttribute::collectionType() const
{
    return d->mType;
}

// Register the attribute when the library is loaded.
namespace
{
bool dummySpecialCollectionAttribute()
{
    using namespace Akonadi;
    AttributeFactory::registerAttribute<SpecialCollectionAttribute>();
    return true;
}

const bool registeredSpecialCollectionAttribute = dummySpecialCollectionAttribute();

} // namespace
