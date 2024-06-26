/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonaditests_export.h"
#include "attribute.h"
#include <QExplicitlySharedDataPointer>
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
class AttributeStoragePrivate;
class AKONADI_TESTS_EXPORT AttributeStorage
{
public:
    AttributeStorage();
    AttributeStorage(const AttributeStorage &other);
    AttributeStorage(AttributeStorage &&other) noexcept;
    AttributeStorage &operator=(const AttributeStorage &other);
    AttributeStorage &operator=(AttributeStorage &&other) noexcept;

    ~AttributeStorage();

    // Detach storage from the shared data
    void detach();

    void addAttribute(Attribute *attr);
    void removeAttribute(const QByteArray &type);
    bool hasAttribute(const QByteArray &type) const;
    [[nodiscard]] Attribute::List attributes() const;
    void clearAttributes();
    const Attribute *attribute(const QByteArray &type) const;
    Attribute *attribute(const QByteArray &type);
    void markAttributeModified(const QByteArray &type);
    void resetChangeLog();

    QSet<QByteArray> deletedAttributes() const;
    [[nodiscard]] bool hasModifiedAttributes() const;
    std::vector<Attribute *> modifiedAttributes() const;

private:
    QExplicitlySharedDataPointer<AttributeStoragePrivate> d;
};

} // namespace Akonadi
