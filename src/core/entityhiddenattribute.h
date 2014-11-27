/******************************************************************************
 *
 *  Copyright (c) 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA, 02110-1301, USA.
 *
 *****************************************************************************/

#ifndef AKONADI_ENTITYHIDDENATTRIBUTE_H
#define AKONADI_ENTITYHIDDENATTRIBUTE_H

#include "akonadicore_export.h"
#include "attribute.h"

namespace Akonadi {

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
 * Collection collection = collectionFetchJob->collections().first();
 * collection.attribute<EntityHiddenAttribute>( Entity::AddIfMissing );
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
    EntityHiddenAttribute();

    /**
     * Destroys the entity hidden attribute.
     */
    ~EntityHiddenAttribute();

    /**
     * Reimplemented from Attribute
     */
    QByteArray type() const Q_DECL_OVERRIDE;

    /**
     * Reimplemented from Attribute
     */
    EntityHiddenAttribute *clone() const Q_DECL_OVERRIDE;

    /**
     * Reimplemented from Attribute
     */
    QByteArray serialized() const Q_DECL_OVERRIDE;

    /**
     * Reimplemented from Attribute
     */
    void deserialize(const QByteArray &data) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
