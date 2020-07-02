/*
    Copyright (c) 2007 - 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ATTRIBUTEFACTORY_H
#define AKONADI_ATTRIBUTEFACTORY_H

#include "akonadicore_export.h"
#include "attribute.h"

namespace Akonadi
{

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
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT AttributeFactory
{
public:
    //@cond PRIVATE
    ~AttributeFactory();
    //@endcond

    /**
     * Registers a custom attribute of type T.
     * The same attribute cannot be registered more than once.
     */
    template <typename T> inline static void registerAttribute()
    {
        AttributeFactory::self()->registerAttribute(new T);
    }

    /**
     * Creates an entity attribute object of the given type.
     * If the type has not been registered, creates a DefaultAttribute.
     *
     * @param type The attribute type.
     */
    static Attribute *createAttribute(const QByteArray &type);

protected:
    //@cond PRIVATE
    AttributeFactory();

private:
    Q_DISABLE_COPY(AttributeFactory)
    static AttributeFactory *self();
    void registerAttribute(Attribute *attribute);

    class Private;
    Private *const d;
    //@endcond
};

}

#endif
