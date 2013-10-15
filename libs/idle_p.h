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

#ifndef AKONADI_IDLE_P_H
#define AKONADI_IDLE_P_H

#include "akonadiprotocolinternals_export.h"

class QByteArray;

namespace Akonadi
{
  namespace Idle {

    enum Operation {
      InvalidOperation,
      Add,
      Modify,
      ModifyFlags,
      Remove,
      Move,
      Link,
      Unlink,
      Subscribe,
      Unsubscribe
    };

    enum Type {
      InvalidType,
      Item,
      Collection
    };

    AKONADIPROTOCOLINTERNALS_EXPORT Idle::Operation commandToOperation( const QByteArray &command );
    AKONADIPROTOCOLINTERNALS_EXPORT QByteArray operationToCommand( Idle::Operation operation );
  }
}

#endif // AKONADI_IDLE_P_H
