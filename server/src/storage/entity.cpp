/***************************************************************************
 *   Copyright (C) 2006 by Andreas Gungl <a.gungl@gmx.de>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "entity.h"
#include "datastore.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtSql/QSqlQuery>

namespace Akonadi {

Entity::Entity()
  : m_id( -1 )
{
}

Entity::Entity( int id )
  : m_id( id )
{
}

int Entity::id() const
{
  return m_id;
}

void Entity::setId( int id )
{
  m_id = id;
}

bool Entity::isValid() const
{
  return m_id != -1;
}

QSqlDatabase Entity::database()
{
  return DataStore::self()->database();
}

}
