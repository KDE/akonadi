/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "idle_p.h"
#include "protocol_p.h"

#include <QByteArray>

using namespace Akonadi;

Idle::IdleOperation Idle::commandToOperation( const QByteArray &command )
{
  if ( command == AKONADI_OPERATION_ADD ) {
    return Akonadi::Idle::Add;
  } else if ( command == AKONADI_OPERATION_MODIFY ) {
    return Akonadi::Idle::Modify;
  } else if ( command == AKONADI_OPERATION_MODIFYFLAGS ) {
    return Akonadi::Idle::ModifyFlags;
  } else if ( command == AKONADI_OPERATION_REMOVE ) {
    return Akonadi::Idle::Remove;
  } else if ( command == AKONADI_OPERATION_MOVE ) {
    return Akonadi::Idle::Move;
  } else if ( command == AKONADI_OPERATION_LINK ) {
    return Akonadi::Idle::Link;
  } else if ( command == AKONADI_OPERATION_UNLINK ) {
    return Akonadi::Idle::Unlink;
  } else if ( command == AKONADI_OPERATION_SUBSCRIBE ) {
    return Akonadi::Idle::Subscribe;
  } else if ( command == AKONADI_OPERATION_UNSUBSCRIBE ) {
    return Akonadi::Idle::Unsubsrcibe;
  }

  return Akonadi::Idle::InvalidOperation;
}

QByteArray Idle::operationToCommand( Idle::IdleOperation operation )
{
  switch ( operation ) {
  case Akonadi::Idle::Add:
    return AKONADI_OPERATION_ADD;
  case Akonadi::Idle::Modify:
    return AKONADI_OPERATION_MODIFY;
  case Akonadi::Idle::ModifyFlags:
    return AKONADI_OPERATION_MODIFYFLAGS;
  case Akonadi::Idle::Remove:
    return AKONADI_OPERATION_REMOVE;
  case Akonadi::Idle::Move:
    return AKONADI_OPERATION_MOVE;
  case Akonadi::Idle::Link:
    return AKONADI_OPERATION_LINK;
  case Akonadi::Idle::Unlink:
    return AKONADI_OPERATION_UNLINK;
  case Akonadi::Idle::Subscribe:
    return AKONADI_OPERATION_SUBSCRIBE;
  case Akonadi::Idle::Unsubsrcibe:
    return AKONADI_OPERATION_UNSUBSCRIBE;
  default:
    return QByteArray();
  }
}
