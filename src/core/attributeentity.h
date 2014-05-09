/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#ifndef AKONADI_ATTRIBUTEENTITY_H
#define AKONADI_ATTRIBUTEENTITY_H

#include "akonadicore_export.h"

#include "job.h"
#include "attribute.h"

#include <QVector>
#include <QSharedPointer>
#include <QDebug>

namespace Akonadi {

/**
 * Parent class for entities that can have attributes.
 * This is supposed to eventually share the code between Akonadi::Tag and Akonadi::Entity.
 *
 * In the current form using this in Akonadi::Entity would break the implicit sharing of it's private class,
 * so AttributeEntity::Private would need to become a parent class of EntityPrivate and use the same clone()
 * calls etc.
 * An even better solution is probably ot make AttributeEntity a private member of Entity, with all Attribute related member functions forwarding to this class.
 */
class AKONADICORE_EXPORT AttributeEntity {
public:
    AttributeEntity();

    AttributeEntity(const AttributeEntity &other);
    virtual ~AttributeEntity();

    //Each subclass must override this to avoid slicing
    virtual AttributeEntity &operator=(const AttributeEntity &other);

    /**
     * Adds an attribute to the entity.
     *
     * If an attribute of the same type name already exists, it is deleted and
     * replaced with the new one.
     *
     * @param attribute The new attribute.
     *
     * @note The entity takes the ownership of the attribute.
     */
    void addAttribute(Attribute *attribute);

    /**
     * Removes and deletes the attribute of the given type @p name.
     */
    void removeAttribute(const QByteArray &name);

    /**
     * Returns @c true if the entity has an attribute of the given type @p name,
     * false otherwise.
     */
    bool hasAttribute(const QByteArray &name) const;

    /**
     * Returns a list of all attributes of the entity.
     */
    Attribute::List attributes() const;

    /**
     * Removes and deletes all attributes of the entity.
     */
    void clearAttributes();

    /**
     * Returns the attribute of the given type @p name if available, 0 otherwise.
     */
    Attribute *attribute(const QByteArray &name) const;

    /**
     * Describes the options that can be passed to access attributes.
     */
    enum CreateOption {
        AddIfMissing    ///< Creates the attribute if it is missing
    };

    /**
     * Returns the attribute of the requested type.
     * If the entity has no attribute of that type yet, a new one
     * is created and added to the entity.
     *
     * @param option The create options.
     */
    template <typename T> inline T *attribute(CreateOption option)
    {
        Q_UNUSED(option);

        const T dummy;
        if (hasAttribute(dummy.type())) {
            T *attr = dynamic_cast<T *>(attribute(dummy.type()));
            if (attr) {
                return attr;
            }
            //reuse 5250
            qWarning() << "Found attribute of unknown type" << dummy.type()
                           << ". Did you forget to call AttributeFactory::registerAttribute()?";
        }

        T *attr = new T();
        addAttribute(attr);
        return attr;
    }

    /**
     * Returns the attribute of the requested type or 0 if it is not available.
     */
    template <typename T> inline T *attribute() const
    {
        const T dummy;
        if (hasAttribute(dummy.type())) {
            T *attr = dynamic_cast<T *>(attribute(dummy.type()));
            if (attr) {
                return attr;
            }
            //Reuse 5250
            qWarning() << "Found attribute of unknown type" << dummy.type()
                           << ". Did you forget to call AttributeFactory::registerAttribute()?";
        }

        return 0;
    }

    /**
     * Removes and deletes the attribute of the requested type.
     */
    template <typename T> inline void removeAttribute()
    {
        const T dummy;
        removeAttribute(dummy.type());
    }

    /**
     * Returns whether the entity has an attribute of the requested type.
     */
    template <typename T> inline bool hasAttribute() const
    {
        const T dummy;
        return hasAttribute(dummy.type());
    }
private:
    friend class TagModifyJob;
    QSet<QByteArray> &removedAttributes() const;

    class Private;
    QSharedPointer<Private> d_ptr;
};

}

#endif
