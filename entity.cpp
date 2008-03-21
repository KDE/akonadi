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

#include "entity.h"

#include "entity_p.h"

using namespace Akonadi;

Entity::Entity()
  : d_ptr( new EntityPrivate )
{
}

Entity::Entity( Id id )
  : d_ptr( new EntityPrivate( id ) )
{
}

Entity::Entity( const Entity &other )
  : d_ptr( other.d_ptr )
{
}

Entity::Entity( EntityPrivate *dd )
  : d_ptr( dd )
{
}

Entity::~Entity()
{
}

Entity::Id Entity::id() const
{
  return d_ptr->mId;
}

void Entity::setRemoteId( const QString& id )
{
  d_ptr->mRemoteId = id;
}

QString Entity::remoteId() const
{
  return d_ptr->mRemoteId;
}

Entity& Entity ::operator=( const Entity &other )
{
  if ( this != &other )
    d_ptr = other.d_ptr;

  return *this;
}

uint qHash( const Akonadi::Entity &entity )
{
  return qHash( entity.id() );
}

AKONADI_DEFINE_PRIVATE( Entity )
