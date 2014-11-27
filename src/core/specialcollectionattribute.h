/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef AKONADI_SPECIALCOLLECTIONATTRIBUTE_P_H
#define AKONADI_SPECIALCOLLECTIONATTRIBUTE_P_H

#include "akonadicore_export.h"
#include "attribute.h"

#include <QtCore/QByteArray>

namespace Akonadi {

/**
 * @short An Attribute that stores the special collection type of a collection.
 *
 * All collections registered with SpecialCollections must have this attribute set.
 *
 * @author Constantin Berzan <exit3219@gmail.com>
 * @since 4.4
 */
class AKONADICORE_EXPORT SpecialCollectionAttribute : public Akonadi::Attribute
{
public:
    /**
     * Creates a new special collection attribute.
     */
    explicit SpecialCollectionAttribute(const QByteArray &type = QByteArray());

    /**
     * Destroys the special collection attribute.
     */
    virtual ~SpecialCollectionAttribute();

    /**
     * Sets the special collections @p type of the collection.
     */
    void setCollectionType(const QByteArray &type);

    /**
     * Returns the special collections type of the collection.
     */
    QByteArray collectionType() const;

    /* reimpl */
    SpecialCollectionAttribute *clone() const Q_DECL_OVERRIDE;
    QByteArray type() const Q_DECL_OVERRIDE;
    QByteArray serialized() const Q_DECL_OVERRIDE;
    void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONATTRIBUTE_H
