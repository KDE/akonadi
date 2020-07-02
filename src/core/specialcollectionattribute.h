/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_SPECIALCOLLECTIONATTRIBUTE_P_H
#define AKONADI_SPECIALCOLLECTIONATTRIBUTE_P_H

#include "akonadicore_export.h"
#include "attribute.h"

#include <QByteArray>

namespace Akonadi
{

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
    ~SpecialCollectionAttribute() override;

    /**
     * Sets the special collections @p type of the collection.
     */
    void setCollectionType(const QByteArray &type);

    /**
     * Returns the special collections type of the collection.
     */
    QByteArray collectionType() const;

    /* reimpl */
    SpecialCollectionAttribute *clone() const override;
    QByteArray type() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_SPECIALCOLLECTIONATTRIBUTE_H
