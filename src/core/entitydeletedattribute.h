/*
 SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

 SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef AKONADI_ENTITYDELETEDATTRIBUTE_H
#define AKONADI_ENTITYDELETEDATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"
#include "collection.h"

namespace Akonadi
{

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
    ~EntityDeletedAttribute() override;
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
    QByteArray type() const override;

    /**
     * Reimplemented from Attribute
     */
    EntityDeletedAttribute *clone() const override;

    /**
     * Reimplemented from Attribute
     */
    QByteArray serialized() const override;

    /**
     * Reimplemented from Attribute
     */
    void deserialize(const QByteArray &data) override;

private:
    //@cond PRIVATE
    class EntityDeletedAttributePrivate;
    EntityDeletedAttributePrivate *const d_ptr;
    Q_DECLARE_PRIVATE(EntityDeletedAttribute)
    //@endcond
};

}

#endif
