/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "attributestorage_p.h"

#include <QSharedData>

using namespace Akonadi;

namespace Akonadi
{

class AttributeStoragePrivate : public QSharedData
{
public:
    AttributeStoragePrivate() = default;
    AttributeStoragePrivate(AttributeStoragePrivate &other)
        : QSharedData(other)
        , atributes(other.atributes)
        , modifiedAttributes(other.modifiedAttributes)
        , deletedAttributes(other.deletedAttributes)
    {
        for (Attribute *attr : std::as_const(atributes)) {
            atributes.insert(attr->type(), attr->clone());
        }
    }

    ~AttributeStoragePrivate()
    {
        qDeleteAll(atributes);
    }

    QHash<QByteArray, Attribute *> atributes;
    std::set<QByteArray> modifiedAttributes;
    QSet<QByteArray> deletedAttributes;
};

} // namespace Akonadi

AttributeStorage::AttributeStorage()
    : d(new AttributeStoragePrivate)
{
}

AttributeStorage::AttributeStorage(const AttributeStorage &other)
    : d(other.d)
{
}

AttributeStorage::AttributeStorage(AttributeStorage &&other) noexcept
{
    d.swap(other.d);
}

AttributeStorage &AttributeStorage::operator=(const AttributeStorage &other)
{
    d = other.d;
    return *this;
}

AttributeStorage &AttributeStorage::operator=(AttributeStorage &&other) noexcept
{
    d.swap(other.d);
    return *this;
}

AttributeStorage::~AttributeStorage() = default;

void AttributeStorage::detach()
{
    d.detach();
}

void AttributeStorage::addAttribute(Attribute *attr)
{
    Q_ASSERT(attr);
    const QByteArray type = attr->type();
    Attribute *existing = d->atributes.value(type);
    if (existing) {
        if (attr == existing) {
            return;
        }
        d->atributes.remove(type);
        delete existing;
    }
    d->atributes.insert(type, attr);
    markAttributeModified(type);
}

void AttributeStorage::removeAttribute(const QByteArray &type)
{
    d->modifiedAttributes.erase(type);
    d->deletedAttributes.insert(type);
    delete d->atributes.take(type);
}

bool AttributeStorage::hasAttribute(const QByteArray &type) const
{
    return d->atributes.contains(type);
}

Attribute::List AttributeStorage::attributes() const
{
    return d->atributes.values();
}

void AttributeStorage::clearAttributes()
{
    for (Attribute *attr : std::as_const(d->atributes)) {
        d->deletedAttributes.insert(attr->type());
        delete attr;
    }
    d->atributes.clear();
    d->modifiedAttributes.clear();
}

const Attribute *AttributeStorage::attribute(const QByteArray &type) const
{
    return d->atributes.value(type);
}

Attribute *AttributeStorage::attribute(const QByteArray &type)
{
    Attribute *attr = d->atributes.value(type);
    if (attr) {
        markAttributeModified(type);
    }
    return attr;
}

void AttributeStorage::markAttributeModified(const QByteArray &type)
{
    if (d->atributes.contains(type)) {
        d->deletedAttributes.remove(type);
        d->modifiedAttributes.insert(type);
    }
}

void AttributeStorage::resetChangeLog()
{
    d->modifiedAttributes.clear();
    d->deletedAttributes.clear();
}

QSet<QByteArray> AttributeStorage::deletedAttributes() const
{
    return d->deletedAttributes;
}

bool AttributeStorage::hasModifiedAttributes() const
{
    return !d->modifiedAttributes.empty();
}

std::vector<Attribute *> AttributeStorage::modifiedAttributes() const
{
    std::vector<Attribute *> ret;
    ret.reserve(d->modifiedAttributes.size());
    for (const auto &type : d->modifiedAttributes) {
        Attribute *attr = d->atributes.value(type);
        Q_ASSERT(attr);
        ret.push_back(attr);
    }
    return ret;
}
