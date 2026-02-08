/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

#include <memory>

namespace Akonadi
{
class CollectionQuotaAttributePrivate;

/*!
 * \class Akonadi::CollectionQuotaAttribute
 * \inheaderfile Akonadi/CollectionQuotaAttribute
 * \inmodule AkonadiCore
 *
 * \brief Attribute that provides quota information for a collection.
 *
 * This attribute class provides quota information (e.g. current fill value
 * and maximum fill value) for an Akonadi collection.
 *
 * Example:
 *
 * \code
 *
 * using namespace Akonadi;
 *
 * const Collection collection = collectionFetchJob->collections().at(0);
 * if ( collection.hasAttribute<CollectionQuotaAttribute>() ) {
 *   const CollectionQuotaAttribute *attribute = collection.attribute<CollectionQuotaAttribute>();
 *   qDebug() << "current value" << attribute->currentValue();
 * }
 *
 * \endcode
 *
 * \author Kevin Ottens <ervin@kde.org>
 * \since 4.4
 */
class AKONADICORE_EXPORT CollectionQuotaAttribute : public Akonadi::Attribute
{
public:
    /*!
     * Creates a new collection quota attribute.
     */
    explicit CollectionQuotaAttribute();

    /*!
     * Creates a new collection quota attribute with initial values.
     *
     * \a currentValue The current quota value in bytes.
     * \a maxValue The maximum quota value in bytes.
     */
    CollectionQuotaAttribute(qint64 currentValue, qint64 maxValue);

    /*!
     * Destroys the collection quota attribute.
     */
    ~CollectionQuotaAttribute() override;

    /*!
     * Sets the current quota \a value for the collection.
     *
     * \a value The current quota value in bytes.
     */
    void setCurrentValue(qint64 value);

    /*!
     * Sets the maximum quota \a value for the collection.
     *
     * \a value The maximum quota value in bytes.
     */
    void setMaximumValue(qint64 value);

    /*!
     * Returns the current quota value in bytes.
     */
    [[nodiscard]] qint64 currentValue() const;

    /*!
     * Returns the maximum quota value in bytes.
     */
    [[nodiscard]] qint64 maximumValue() const;

    /*!
     */
    QByteArray type() const override;
    /*!
     */
    Attribute *clone() const override;
    /*!
     */
    [[nodiscard]] QByteArray serialized() const override;
    /*!
     */
    void deserialize(const QByteArray &data) override;

private:
    const std::unique_ptr<CollectionQuotaAttributePrivate> d;
};

} // namespace Akonadi
