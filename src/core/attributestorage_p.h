/*
    Copyright (c) 2019 David Faure <faure@kde.org>

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

#ifndef ATTRIBUTESTORAGE_P_H
#define ATTRIBUTESTORAGE_P_H

#include <set>
#include <vector>
#include <QHash>
#include <QSet>
#include "attribute.h"

namespace Akonadi {

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
    AttributeStorage& operator=(const AttributeStorage &other);
    void swap(AttributeStorage &other) noexcept;
    ~AttributeStorage();

    void addAttribute(Attribute *attr);
    void removeAttribute(const QByteArray &type);
    bool hasAttribute(const QByteArray &type) const;
    Attribute::List attributes() const;
    void clearAttributes();
    const Attribute *attribute(const QByteArray &type) const;
    Attribute *attribute(const QByteArray &type);
    void markAttributeModified(const QByteArray &type);
    void resetChangeLog();

    QSet<QByteArray> deletedAttributes() const;
    bool hasModifiedAttributes() const;
    std::vector<Attribute *> modifiedAttributes() const;

private:
    QHash<QByteArray, Attribute *> mAttributes;
    std::set<QByteArray> mModifiedAttributes;
    QSet<QByteArray> mDeletedAttributes;
};

}

#endif // ATTRIBUTESTORAGE_P_H
