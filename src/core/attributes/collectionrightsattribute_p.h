/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONRIGHTSATTRIBUTE_P_H
#define AKONADI_COLLECTIONRIGHTSATTRIBUTE_P_H

#include "akonadicore_export.h"

#include "collection.h"
#include "attribute.h"

#include <memory>

namespace Akonadi
{

/**
 * @internal
 *
 * @short Attribute that stores the rights of a collection.
 *
 * Every collection can have rights set which describes whether
 * the collection is readable or writable. That information is stored
 * in this custom attribute.
 *
 * @note You shouldn't use this class directly but the convenience methods
 * Collection::rights() and Collection::setRights() instead.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT CollectionRightsAttribute : public Attribute
{
public:
    /**
     * Creates a new collection rights attribute.
     */
    explicit CollectionRightsAttribute();

    /**
     * Destroys the collection rights attribute.
     */
    ~CollectionRightsAttribute() override;

    /**
     * Sets the @p rights of the collection.
     */
    void setRights(Collection::Rights rights);

    /**
     * Returns the rights of the collection.
     */
    Collection::Rights rights() const;

    QByteArray type() const override;

    CollectionRightsAttribute *clone() const override;

    QByteArray serialized() const override;

    void deserialize(const QByteArray &data) override;

private:
    //@cond PRIVATE
    class Private;
    const std::unique_ptr<Private> d;
    //@endcond
};

} // namespace Akonadi

#endif
