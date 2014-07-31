/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLMOVE_H
#define AKONADI_COLMOVE_H

#include <handler.h>
#include "scope.h"

namespace Akonadi {
namespace Server {

/**
  @ingroup akonadi_server_handler

  Handler for the COLMOVE command.

  This command is used to move a set of collections into another collection, including
  all sub-collections and their content.

  <h4>Syntax</h4>

  Request:
  @verbatim
  request = tag [" RID"] " COLMOVE " collection-ids " " collection-id
  @endverbatim

  @c collection-ids is the set of collections that should be moved, either as UID-set
  or as a list of RIDs (in case the @c RID prefix is given).

  @c collection-id is a single collection UID and describes the target collection.

  There is only the usual status response indicating success or failure of the
  COLMOVE command
*/
class ColMove : public Handler
{
    Q_OBJECT
public:
    ColMove(Scope::SelectionScope scope);
    virtual bool parseStream();

private:
    Scope m_scope;
};

} // namespace Server
} // namespace Akonadi

#endif
