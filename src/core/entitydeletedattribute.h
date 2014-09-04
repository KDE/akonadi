/*
 Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#ifndef AKONADI_ENTITYDELETEDATTRIBUTE_H
#define AKONADI_ENTITYDELETEDATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"
#include "collection.h"

namespace Akonadi {

/**
 * @short An Attribute that marks that an entity was marked as deleted
 *
 * This class represents the attribute of all hidden items. The hidden
 * items shouldn't be displayed in UI applications (unless in some kind
 * of "debug" mode).
 *
 * Example:
 *
 * @code
 *
 * @endcode
 *
 * @author Christian Mollekopf <chrigi_1@fastmail.fm>
 * @see Akonadi::Attribute
 * @since 4.8
 */
class AKONADICORE_EXPORT EntityDeletedAttribute : public Attribute
{
public:
    /**
     * Creates a new entity deleted attribute.
     */
    EntityDeletedAttribute();

    /**
     * Destroys the entity deleted attribute.
     */
    ~EntityDeletedAttribute();
    /**
     * Sets the collection used to restore items which have been moved to trash using a TrashJob
     * If the Resource is set on the collection, the resource root will be used as fallback during the restore operation.
     */
    void setRestoreCollection(const Collection &col);

    /**
     * Returns the original collection of an item that has been moved to trash using a TrashJob
     */
    Collection restoreCollection() const;

    /**
     * Returns the resource of the restoreCollection
     */
    QString restoreResource() const;

    /**
     * Reimplemented from Attribute
     */
    QByteArray type() const;

    /**
     * Reimplemented from Attribute
     */
    EntityDeletedAttribute *clone() const;

    /**
     * Reimplemented from Attribute
     */
    QByteArray serialized() const;

    /**
     * Reimplemented from Attribute
     */
    void deserialize(const QByteArray &data);

private:
    //@cond PRIVATE
    class EntityDeletedAttributePrivate;
    EntityDeletedAttributePrivate *const d_ptr;
    Q_DECLARE_PRIVATE(EntityDeletedAttribute)
    //@endcond
};

}

#endif
