/*
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

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

#ifndef AKONADI_COLLECTIONQUOTAATTRIBUTE_H
#define AKONADI_COLLECTIONQUOTAATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"

namespace Akonadi {

/**
 * @short Attribute that provides quota information for a collection.
 *
 * This attribute class provides quota information (e.g. current fill value
 * and maximum fill value) for an Akonadi collection.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * const Collection collection = collectionFetchJob->collections().first();
 * if ( collection.hasAttribute<CollectionQuotaAttribute>() ) {
 *   const CollectionQuotaAttribute *attribute = collection.attribute<CollectionQuotaAttribute>();
 *   qDebug() << "current value" << attribute->currentValue();
 * }
 *
 * @endcode
 *
 * @author Kevin Ottens <ervin@kde.org>
 * @since 4.4
 */
class AKONADICORE_EXPORT CollectionQuotaAttribute : public Akonadi::Attribute
{
public:
    /**
     * Creates a new collection quota attribute.
     */
    CollectionQuotaAttribute();

    /**
     * Creates a new collection quota attribute with initial values.
     *
     * @param currentValue The current quota value in bytes.
     * @param maxValue The maximum quota value in bytes.
     */
    CollectionQuotaAttribute(qint64 currentValue, qint64 maxValue);

    /**
     * Destroys the collection quota attribute.
     */
    ~CollectionQuotaAttribute();

    /**
     * Sets the current quota @p value for the collection.
     *
     * @param value The current quota value in bytes.
     */
    void setCurrentValue(qint64 value);

    /**
     * Sets the maximum quota @p value for the collection.
     *
     * @param value The maximum quota value in bytes.
     */
    void setMaximumValue(qint64 value);

    /**
     * Returns the current quota value in bytes.
     */
    qint64 currentValue() const;

    /**
     * Returns the maximum quota value in bytes.
     */
    qint64 maximumValue() const;

    QByteArray type() const Q_DECL_OVERRIDE;
    Attribute *clone() const Q_DECL_OVERRIDE;
    QByteArray serialized() const Q_DECL_OVERRIDE;
    void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
