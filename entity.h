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

#include <QtCore/QHash>
#include <QtCore/QSharedDataPointer>

#define AKONADI_DECLARE_PRIVATE( Class ) \
    Class##Private* d_func(); \
    const Class##Private* d_func() const; \
    friend class Class##Private;

namespace Akonadi {

class EntityPrivate;

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
     * @internal
     */
    Entity& operator=( const Entity &other );

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
