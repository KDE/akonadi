/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ENTITY_H
#define AKONADI_ENTITY_H

#include "akonadicore_export.h"
#include <QString>

namespace Akonadi {
class Entity;
}

AKONADICORE_EXPORT uint qHash(const Akonadi::Entity &);

#include "attribute.h"

#include <QDebug>

#include <QtCore/QHash>
#include <QtCore/QSharedDataPointer>

#define AKONADI_DECLARE_PRIVATE( Class ) \
    Class##Private *d_func(); \
    const Class##Private *d_func() const; \
    friend class Class##Private;

namespace Akonadi {

class Collection;
class EntityPrivate;

/**
 * @short The base class for Item and Collection.
 *
 * Entity is the common base class for Item and Collection that provides
 * unique IDs and attributes handling.
 *
 * This class is not meant to be used directly, use Item or Collection instead.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADICORE_EXPORT Entity
{
public:
    /**
     * Describes the unique id type.
     */
    typedef qint64 Id;

    /**
     * Sets the unique @p identifier of the entity.
     */
    void setId(Id identifier);

    /**
     * Returns the unique identifier of the entity.
     */
    Id id() const;

    /**
     * Sets the remote @p id of the entity.
     */
    void setRemoteId(const QString &id);

    /**
     * Returns the remote id of the entity.
     */
    QString remoteId() const;

    /**
     * Sets the remote @p revision of the entity.
     * @param revision the entity's remote revision
     * The remote revision can be used by resources to store some
     * revision information of the backend to detect changes there.
     *
     * @note This method is supposed to be used by resources only.
     * @since 4.5
     */
    void setRemoteRevision(const QString &revision);

    /**
     * Returns the remote revision of the entity.
     *
     * @note This method is supposed to be used by resources only.
     * @since 4.5
     */
    QString remoteRevision() const;

    /**
     * Returns whether the entity is valid.
     */
    bool isValid() const;

    /**
     * Returns whether the entity's id equals the
     * id of the @p other entity.
     */
    bool operator==(const Entity &other) const;

    /**
     * Returns whether the entity's id does not equal the id
     * of the @p other entity.
     */
    bool operator!=(const Entity &other) const;

    /**
     * Assigns the @p other to this entity and returns a reference to this entity.
     * @param other the entity to assign
     */
    Entity &operator=(const Entity &other);

    /**
     * @internal For use with containers only.
     *
     * @since 4.8
     */
    bool operator<(const Entity &other) const;

    /**
     * Returns the parent collection of this object.
     * @note This will of course only return a useful value if it was explictly retrieved
     *       from the Akonadi server.
     * @since 4.4
     */
    Collection parentCollection() const;

    /**
     * Returns a reference to the parent collection of this object.
     * @note This will of course only return a useful value if it was explictly retrieved
     *       from the Akonadi server.
     * @since 4.4
     */
    Collection &parentCollection();

    /**
     * Set the parent collection of this object.
     * @note Calling this method has no immediate effect for the object itself,
     *       such as being moved to another collection.
     *       It is mainly relevant to provide a context for RID-based operations
     *       inside resources.
     * @param parent The parent collection.
     * @since 4.4
     */
    void setParentCollection(const Collection &parent);

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
            //Reuse 5250
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
            //reuse 5250
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

protected:
    /**
     * Creates an entity from an @p other entity.
     */
    Entity(const Entity &other);

    /**
     * Destroys the entity.
     */
    ~Entity();

    //@cond PRIVATE
    Entity(EntityPrivate *dd);
    QSharedDataPointer<EntityPrivate> d_ptr;
    //@endcond

    AKONADI_DECLARE_PRIVATE(Entity)
};

}

#endif
