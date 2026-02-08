/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

#include <memory>

namespace Akonadi
{
class IndexPolicyAttributePrivate;

/*!
 * \brief An attribute to specify how a collection should be indexed for searching.
 *
 * This attribute can be attached to any collection and should be honored by indexing
 * agents.
 *
 * \class Akonadi::IndexPolicyAttribute
 * \inheaderfile Akonadi/IndexPolicyAttribute
 * \inmodule AkonadiCore
 *
 * \since 4.6
 */
class AKONADICORE_EXPORT IndexPolicyAttribute : public Akonadi::Attribute
{
public:
    /*!
     * Creates a new index policy attribute.
     */
    IndexPolicyAttribute();

    /*!
     * Destroys the index policy attribute.
     */
    ~IndexPolicyAttribute() override;

    /*!
     * Returns whether this collection is supposed to be indexed at all.
     */
    [[nodiscard]] bool indexingEnabled() const;

    /*!
     * Sets whether this collection should be indexed at all.
     * \a enable \\ true to enable indexing, \\ false to exclude this collection from indexing
     */
    void setIndexingEnabled(bool enable);

    [[nodiscard]] QByteArray type() const override;
    Attribute *clone() const override;
    [[nodiscard]] QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    const std::unique_ptr<IndexPolicyAttributePrivate> d;
};

} // namespace Akonadi
