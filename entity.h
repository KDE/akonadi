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

#include "akonadi_export.h"

namespace Akonadi {
class Entity;
}

AKONADI_EXPORT uint qHash( const Akonadi::Entity& );

#include <akonadi/attribute.h>

#include <QtCore/QHash>
#include <QtCore/QSharedDataPointer>

#define AKONADI_DECLARE_PRIVATE( Class ) \
    Class##Private* d_func(); \
    const Class##Private* d_func() const; \
    friend class Class##Private;

namespace Akonadi {

class EntityPrivate;

/**
 * @short The base class for Item and Collection.
 */
class AKONADI_EXPORT Entity
{
  public:
    /**
     * Describes the unique id type.
     */
    typedef qint64 Id;

    /**
     * Destroys the entity.
     */
    ~Entity();

    /**
     * Sets the unique identifier of the entity.
     */
    void setId( Id id );

    /**
     * Returns the unique identifier of the entity.
     */
    Id id() const;

    /**
     * Sets the remote @p id of the entity.
     */
    void setRemoteId( const QString& id );

    /**
     * Returns the remote id of the entity.
     */
    QString remoteId() const;

    /**
     * Returns whether the entity is valid.
     */
    bool isValid() const;

    /**
     * Returns whether the entity's id equals the
     * id of the @p other entity.
     */
    bool operator==( const Entity &other ) const;

    /**
     * Returns whether the entity's id does not equal the id
     * of the @p other entity.
     */
    bool operator!=( const Entity &other ) const;

    /**
     * Assignment operator.
     */
    Entity& operator=( const Entity &other );

    /**
      Adds an attribute. An already existing attribute of the same
      type is deleted.
      @param attr The new attribute. This object takes the ownership of the attribute.
    */
    void addAttribute( Attribute *attr );

    /**
      Removes an attribute. If an attribute with the given type exists,
      it will be deleted.
      @param type The attribute type to remove.
    */
    void removeAttribute( const QByteArray &type );

    /**
      Returns true if the entity has the specified attribute.
      @param type The attribute type.
    */
    bool hasAttribute( const QByteArray &type ) const;

    /**
      Returns all attributes.
    */
    Attribute::List attributes() const;

    /**
      Removes all attributes of this object. The attributes are deleted.
    */
    void clearAttributes();

    /**
      Returns the attribute of the given type if available, 0 otherwise.
      @param type The attribute type.
    */
    Attribute* attribute( const QByteArray &type ) const;

    //FIXME_API: maybe better naming...
    enum CreateOption
    {
      AddIfMissing
    };

    /**
      Returns the attribute of the requested type or 0 if not available.
      @param create Creates the attribute if it doesn't exist.
    */
    template <typename T> inline T* attribute( CreateOption create )
    {
      Q_UNUSED( create );

      const T dummy;
      if ( hasAttribute( dummy.type() ) )
        return static_cast<T*>( attribute( dummy.type() ) );

      T* attr = new T();
      addAttribute( attr );
      return attr;
    }

    /**
      Returns the attribute of the requested type or 0 if not available.
    */
    template <typename T> inline T* attribute() const
    {
      const T dummy;
      if ( hasAttribute( dummy.type() ) )
        return static_cast<T*>( attribute( dummy.type() ) );
      return 0;
    }

    /**
      Removes and deletes the attribute of type @p T, if any.
    */
    template <typename T> inline void removeAttribute()
    {
      const T dummy;
      removeAttribute( dummy.type() );
    }

    /**
      Checks if this entity has an attribute of type @p T.
    */
    template <typename T> inline bool hasAttribute()
    {
      const T dummy;
      return hasAttribute( dummy.type() );
    }

  protected:
    /**
     * Creates an entity from an @p other entity.
     */
    Entity( const Entity &other );

    //@cond PRIVATE
    Entity( EntityPrivate *dd );
    QSharedDataPointer<EntityPrivate> d_ptr;
    //@endcond

    AKONADI_DECLARE_PRIVATE( Entity )
};

}

#endif
