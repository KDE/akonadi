/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "attributestorage_p.h"

using namespace Akonadi;

AttributeStorage::AttributeStorage()
{
}

AttributeStorage::AttributeStorage(const AttributeStorage &other)
    : mModifiedAttributes(other.mModifiedAttributes)
    , mDeletedAttributes(other.mDeletedAttributes)
{
    for (Attribute *attr : qAsConst(other.mAttributes)) {
        mAttributes.insert(attr->type(), attr->clone());
    }
}

AttributeStorage &AttributeStorage::operator=(const AttributeStorage &other)
{
    AttributeStorage copy(other);
    swap(copy);
    return *this;
}

void AttributeStorage::swap(AttributeStorage &other) noexcept
{
    using std::swap;
    swap(other.mAttributes, mAttributes);
    swap(other.mModifiedAttributes, mModifiedAttributes);
    swap(other.mDeletedAttributes, mDeletedAttributes);
}

AttributeStorage::~AttributeStorage()
{
    qDeleteAll(mAttributes);
}

void AttributeStorage::addAttribute(Attribute *attr)
{
    Q_ASSERT(attr);
    const QByteArray type = attr->type();
    Attribute *existing = mAttributes.value(type);
    if (existing) {
        if (attr == existing) {
            return;
        }
        mAttributes.remove(type);
        delete existing;
    }
    mAttributes.insert(type, attr);
    markAttributeModified(type);
}

void AttributeStorage::removeAttribute(const QByteArray &type)
{
    mModifiedAttributes.erase(type);
    mDeletedAttributes.insert(type);
    delete mAttributes.take(type);
}

bool AttributeStorage::hasAttribute(const QByteArray &type) const
{
    return mAttributes.contains(type);
}

Attribute::List AttributeStorage::attributes() const
{
    return mAttributes.values();
}

void AttributeStorage::clearAttributes()
{
    for (Attribute *attr : qAsConst(mAttributes)) {
        mDeletedAttributes.insert(attr->type());
        delete attr;
    }
    mAttributes.clear();
    mModifiedAttributes.clear();
}

const Attribute *AttributeStorage::attribute(const QByteArray &type) const
{
    return mAttributes.value(type);
}

Attribute *AttributeStorage::attribute(const QByteArray &type)
{
    Attribute *attr = mAttributes.value(type);
    if (attr) {
        markAttributeModified(type);
    }
    return attr;
}

void AttributeStorage::markAttributeModified(const QByteArray &type)
{
    if (mAttributes.contains(type)) {
        mDeletedAttributes.remove(type);
        mModifiedAttributes.insert(type);
    }
}

void AttributeStorage::resetChangeLog()
{
    mModifiedAttributes.clear();
    mDeletedAttributes.clear();
}

QSet<QByteArray> AttributeStorage::deletedAttributes() const
{
    return mDeletedAttributes;
}

bool AttributeStorage::hasModifiedAttributes() const
{
    return !mModifiedAttributes.empty();
}

std::vector<Attribute *> AttributeStorage::modifiedAttributes() const
{
    std::vector<Attribute *> ret;
    ret.reserve(mModifiedAttributes.size());
    for (const auto &type : mModifiedAttributes) {
        Attribute *attr = mAttributes.value(type);
        Q_ASSERT(attr);
        ret.push_back(attr);
    }
    return ret;
}
