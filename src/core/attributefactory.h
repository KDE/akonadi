/*
    SPDX-FileCopyrightText: 2007-2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "attribute.h"

#include <memory>

namespace Akonadi
{
class AttributeFactoryPrivate;

/**
 * @short Provides the functionality of registering and creating arbitrary
 *        entity attributes.
 *
 * This class provides the functionality of registering and creating arbitrary Attributes for Entity
 * and its subclasses (e.g. Item and Collection).
 *
 * @code
 *
 * // register the type first
 * Akonadi::AttributeFactory::registerAttribute<SecrecyAttribute>();
 *
 * ...
 *
 * // use it anywhere else in the application
 * SecrecyAttribute *attr = Akonadi::AttributeFactory::createAttribute( "secrecy" );
 *
 * @endcode
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT AttributeFactory
{
public:
    /// @cond PRIVATE
    ~AttributeFactory();
    /// @endcond

    /**
     * Registers a custom attribute of type T.
     * The same attribute cannot be registered more than once.
     */
    template<typename T>
    inline static void registerAttribute()
    {
        static_assert(std::is_default_constructible<T>::value, "An Attribute must be default-constructible.");
        AttributeFactory::self()->registerAttribute(std::unique_ptr<T>{new T{}});
    }

    /**
     * Creates an entity attribute object of the given type.
     * If the type has not been registered, creates a DefaultAttribute.
     *
     * @param type The attribute type.
     */
    static Attribute *createAttribute(const QByteArray &type);

protected:
    /// @cond PRIVATE
    explicit AttributeFactory();

private:
    Q_DISABLE_COPY(AttributeFactory)
    static AttributeFactory *self();
    void registerAttribute(std::unique_ptr<Attribute> attribute);

    const std::unique_ptr<AttributeFactoryPrivate> d;
    /// @endcond
};

} // namespace Akonadi
