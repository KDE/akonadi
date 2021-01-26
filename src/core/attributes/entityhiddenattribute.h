/******************************************************************************
 *
 *  SPDX-FileCopyrightText: 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 *
 *****************************************************************************/

#ifndef AKONADI_ENTITYHIDDENATTRIBUTE_H
#define AKONADI_ENTITYHIDDENATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"

#include <memory>

namespace Akonadi
{
/**
 * @short An Attribute that marks that an entity should be hidden in the UI.
 *
 * This class represents the attribute of all hidden items. The hidden
 * items shouldn't be displayed in UI applications (unless in some kind
 * of "debug" mode).
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * ...
 * // hide a collection by setting the hidden attribute
 * Collection collection = collectionFetchJob->collections().at(0);
 * collection.attribute<EntityHiddenAttribute>( Collection::AddIfMissing );
 * new CollectionModifyJob( collection, this ); // save back to storage
 *
 * // check if the collection is hidden
 * if ( collection.hasAttribute<EntityHiddenAttribute>() )
 *   qDebug() << "collection is hidden";
 * else
 *   qDebug() << "collection is visible";
 *
 * @endcode
 *
 * @author Szymon Stefanek <s.stefanek@gmail.com>
 * @see Akonadi::Attribute
 * @since 4.4
 */
class AKONADICORE_EXPORT EntityHiddenAttribute : public Attribute
{
public:
    /**
     * Creates a new entity hidden attribute.
     */
    explicit EntityHiddenAttribute();

    /**
     * Destroys the entity hidden attribute.
     */
    ~EntityHiddenAttribute() override;

    /**
     * Reimplemented from Attribute
     */
    QByteArray type() const override;

    /**
     * Reimplemented from Attribute
     */
    EntityHiddenAttribute *clone() const override;

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
    class Private;
    const std::unique_ptr<Private> d;
    //@endcond
};

} // namespace Akonadi

#endif
