/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"

#include "noop.h"

using namespace Akonadi;

Noop::Noop()
  : Handler()
{
}

Noop::~Noop()
{
}

bool Noop::handleLine( const QByteArray& )
{
  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( "NOOP completed" );

  connection()->flushStatusMessageQueue();

  emit responseAvailable( response );

  deleteLater();

  return true;
}
