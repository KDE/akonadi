/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "attribute.h"
#include <QHash>
#include <QSet>
#include <set>
#include <vector>

namespace Akonadi
{
/**
 * The AttributeStorage class is used by Collection, Item, Tag...
 * to store a set of attributes, remembering modifications.
 * I.e. it knows which attributes have been added or removed
 * compared to the initial set (e.g. fetched from server).
 */
class AttributeStorage
{
public:
    AttributeStorage();
    AttributeStorage(const AttributeStorage &other);
    AttributeStorage &operator=(const AttributeStorage &other);
    void swap(AttributeStorage &other) noexcept;
    ~AttributeStorage();

    void addAttribute(Attribute *attr);
    void removeAttribute(const QByteArray &type);
    bool hasAttribute(const QByteArray &type) const;
    Q_REQUIRED_RESULT Attribute::List attributes() const;
    void clearAttributes();
    const Attribute *attribute(const QByteArray &type) const;
    Attribute *attribute(const QByteArray &type);
    void markAttributeModified(const QByteArray &type);
    void resetChangeLog();

    QSet<QByteArray> deletedAttributes() const;
    Q_REQUIRED_RESULT bool hasModifiedAttributes() const;
    std::vector<Attribute *> modifiedAttributes() const;

private:
    QHash<QByteArray, Attribute *> mAttributes;
    std::set<QByteArray> mModifiedAttributes;
    QSet<QByteArray> mDeletedAttributes;
};

}

