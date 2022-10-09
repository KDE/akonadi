/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

#include <QByteArray>

#include <memory>

namespace Akonadi
{
class SpecialCollectionAttributePrivate;

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
    /// @cond PRIVATE
    const std::unique_ptr<SpecialCollectionAttributePrivate> d;
    /// @endcond
};

} // namespace Akonadi
